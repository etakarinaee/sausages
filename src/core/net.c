#include "net.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define HEADER 5

enum {
    PACKET_CONNECT,
    PACKET_CONNECT_ACKNOWLEDGMENT,
    PACKET_DISCONNECT,
    PACKET_DATA,
    PACKET_VOICE,
};

static double net_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static int udp_sock(const char *ip, const uint16_t port) {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    const int opt = 1;
    int fl;

    if (fd < 0) {
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    if ((fl = fcntl(fd, F_GETFL, 0)) >= 0) {
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    }

    if (port) {
        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr.s_addr = ip ? inet_addr(ip) : INADDR_ANY,
        };

        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(fd);

            return -1;
        }
    }

    return fd;
}

static uint32_t resolve(const char *host, const uint16_t port,
                        struct net_addr *addr) {
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
    };
    struct addrinfo *res;

    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        return 0;
    }

    *addr = (struct net_addr){
        .host = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr,
        .port = htons(port)};
    freeaddrinfo(res);

    return 1;
}

static void udp_send(const int fd, const struct net_addr *to, const void *data,
                     const uint32_t len) {
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = to->port,
        .sin_addr.s_addr = to->host,
    };

    sendto(fd, data, len, 0, (struct sockaddr *)&addr, sizeof(addr));
}

static int udp_recv(const int fd, struct net_addr *from, void *buf,
                    const uint32_t max) {
    struct sockaddr_in addr;
    socklen_t alen = sizeof(addr);

    const int n =
        (int)recvfrom(fd, buf, max, 0, (struct sockaddr *)&addr, &alen);
    if (n <= 0) {
        return -1;
    }

    *from = (struct net_addr){
        .host = addr.sin_addr.s_addr,
        .port = addr.sin_port,
    };

    return n;
}

static void packet_pack(uint8_t *buf, const uint32_t type) {
    const uint32_t id = NET_PROTOCOL_ID;
    buf[0] = (uint8_t)id;
    buf[1] = (uint8_t)(id >> 8);
    buf[2] = (uint8_t)(id >> 16);
    buf[3] = (uint8_t)(id >> 24);
    buf[4] = (uint8_t)type;
}

static uint32_t packet_check(const uint8_t *buf, const int n, uint32_t *type) {
    if (n < HEADER) {
        return 0;
    }

    const uint32_t id =
        buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    if (id != NET_PROTOCOL_ID) {
        return 0;
    }

    *type = buf[4];

    return 1;
}

static void packet_send(const int fd, const struct net_addr *to,
                        const uint32_t type) {
    uint8_t buf[HEADER];
    packet_pack(buf, type);
    udp_send(fd, to, buf, HEADER);
}

static void send_acknowledgment(const int fd, const struct net_addr *to,
                                const uint32_t id) {
    uint8_t buf[HEADER + 4];
    packet_pack(buf, PACKET_CONNECT_ACKNOWLEDGMENT);
    buf[5] = id & 0xff;
    buf[6] = id >> 8 & 0xff;
    buf[7] = id >> 16 & 0xff;
    buf[8] = id >> 24 & 0xff;

    udp_send(fd, to, buf, HEADER + 4);
}

static uint32_t addr_eq(const struct net_addr *a, const struct net_addr *b) {
    return a->host == b->host && a->port == b->port;
}

static uint32_t peer_find(const struct net_server *server,
                          const struct net_addr *addr) {
    for (uint32_t i = 0; i < server->max_clients; i++) {
        if (server->peers[i].alive && addr_eq(&server->peers[i].addr, addr)) {
            return i;
        }
    }

    return UINT32_MAX;
}

static uint32_t peer_alloc(const struct net_server *server) {
    for (uint32_t i = 0; i < server->max_clients; i++) {
        if (!server->peers[i].alive) {
            return i;
        }
    }

    return UINT32_MAX;
}

static net_voice_recv_fn voice_recv_callback = NULL;

void net_set_voice_callback(const net_voice_recv_fn callback) {
    voice_recv_callback = callback;
}

struct net_server *net_server_create(const char *ip, const uint16_t port,
                                     uint32_t n) {
    if (n < 1) {
        n = 1;
    }

    if (n > 1024) {
        n = 1024;
    }

    struct net_server *server = calloc(1, sizeof(*server));
    if (!server) {
        return NULL;
    }

    server->peers = calloc(n, sizeof(*server->peers));
    if (!server->peers) {
        free(server);

        return NULL;
    }

    server->fd = udp_sock(ip, port);
    if (server->fd < 0) {
        free(server->peers);
        free(server);

        return NULL;
    }

    server->max_clients = n;
    server->n = 0;
    server->last_sweep = net_time();

    return server;
}

void net_server_destroy(struct net_server *server) {
    if (!server) {
        return;
    }

    for (uint32_t i = 0; i < server->max_clients; i++) {
        if (server->peers[i].alive) {
            packet_send(server->fd, &server->peers[i].addr, PACKET_DISCONNECT);
        }
    }

    close(server->fd);
    free(server->peers);

    free(server);
}

uint32_t net_server_poll(struct net_server *server, struct net_event *event) {
    uint8_t buf[HEADER + NET_PAYLOAD];
    struct net_addr from;
    const double t = net_time();
    uint32_t type, id;

    if (t - server->last_sweep > 1.0) {
        for (; server->sweep_index < server->max_clients;
             server->sweep_index++) {
            id = server->sweep_index;

            if (server->peers[id].alive &&
                t - server->peers[id].last_recv > NET_TIMEOUT) {
                server->peers[id].alive = false;
                server->n--;
                server->sweep_index++;

                *event = (struct net_event){
                    .type = NET_EVENT_DISCONNECT,
                    .client_id = id,
                };

                return 1;
            }
        }

        server->sweep_index = 0;
        server->last_sweep = t;
    }

    const int n = udp_recv(server->fd, &from, buf, sizeof(buf));
    if (n < 0 || !packet_check(buf, n, &type)) {
        return 0;
    }

    if (type == PACKET_CONNECT) {
        id = peer_find(server, &from);
        if (id != UINT32_MAX) {
            server->peers[id].last_recv = t;
            send_acknowledgment(server->fd, &from, id);

            return 0;
        }

        id = peer_alloc(server);
        if (id == UINT32_MAX) {
            return 0;
        }

        server->peers[id] = (struct net_peer){
            .addr = from,
            .alive = true,
            .last_recv = t,
        };
        server->n++;

        send_acknowledgment(server->fd, &from, id);

        *event = (struct net_event){
            .type = NET_EVENT_CONNECT,
            .client_id = id,
            .len = 0,
        };

        return 1;
    }

    if (type == PACKET_DISCONNECT) {
        id = peer_find(server, &from);
        if (id == UINT32_MAX) {
            return 0;
        }

        server->peers[id].alive = false;
        server->n--;

        *event = (struct net_event){
            .type = NET_EVENT_DISCONNECT,
            .client_id = id,
            .len = 0,
        };

        return 1;
    }

    if (type == PACKET_DATA) {
        id = peer_find(server, &from);
        if (id == UINT32_MAX) {
            return 0;
        }

        server->peers[id].last_recv = t;

        const uint32_t payload = (uint32_t)(n - HEADER);

        *event = (struct net_event){
            .type = NET_EVENT_DATA,
            .client_id = id,
            .len = payload,
        };
        memcpy(event->data, buf + HEADER, payload);

        return 1;
    }

    if (type == PACKET_VOICE) {
        id = peer_find(server, &from);

        if (id == UINT32_MAX) {
            return 0;
        }

        server->peers[id].last_recv = t;

        uint8_t relay[HEADER + 4 + NET_PAYLOAD];
        packet_pack(relay, PACKET_VOICE);
        relay[5] = (uint8_t)(id);
        relay[6] = (uint8_t)(id >> 8);
        relay[7] = (uint8_t)(id >> 16);
        relay[8] = (uint8_t)(id >> 24);

        const uint32_t payload = (uint32_t)(n - HEADER);
        if (payload > 0 && payload <= NET_PAYLOAD) {
            memcpy(relay + HEADER + 4, buf + HEADER, payload);
        }

        const uint32_t total = HEADER + 4 + payload;
        for (uint32_t i = 0; i < server->max_clients; i++) {
            if (i != id && server->peers[i].alive) {
                udp_send(server->fd, &server->peers[i].addr, relay, total);
            }
        }
        return 0;
    }

    return 0;
}

void net_server_send(const struct net_server *server, uint32_t client_id,
                     const void *data, uint32_t len) {
    if (client_id >= server->max_clients || !server->peers[client_id].alive) {
        return;
    }

    if (len > NET_PAYLOAD) {
        len = NET_PAYLOAD;
    }

    uint8_t buf[HEADER + NET_PAYLOAD];
    packet_pack(buf, PACKET_DATA);
    memcpy(buf + HEADER, data, len);

    udp_send(server->fd, &server->peers[client_id].addr, buf, HEADER + len);
}

void net_server_broadcast(const struct net_server *server, const void *data,
                          uint32_t len) {
    if (len > NET_PAYLOAD)
        len = NET_PAYLOAD;

    uint8_t buf[HEADER + NET_PAYLOAD];
    packet_pack(buf, PACKET_DATA);
    memcpy(buf + HEADER, data, len);

    const uint32_t total = HEADER + len;

    for (uint32_t i = 0; i < server->max_clients; i++) {
        if (server->peers[i].alive) {
            udp_send(server->fd, &server->peers[i].addr, buf, total);
        }
    }
}

struct net_client *net_client_create(const char *host, uint16_t port) {
    struct net_client *client = calloc(1, sizeof(*client));
    if (!client) {
        return NULL;
    }

    if (!resolve(host, port, &client->server)) {
        free(client);

        return NULL;
    }

    client->fd = udp_sock(NULL, 0);

    if (client->fd < 0) {
        free(client);
        return NULL;
    }

    client->connecting = true;

    return client;
}

void net_client_destroy(struct net_client *client) {
    if (!client) {
        return;
    }

    if (client->connected) {
        net_client_disconnect(client);
    }

    close(client->fd);
    free(client);
}

void net_client_disconnect(struct net_client *client) {
    if (client->connected) {
        for (uint32_t i = 0; i < 3; i++) {
            packet_send(client->fd, &client->server, PACKET_DISCONNECT);
        }
    }

    client->connected = false;
    client->connecting = false;
}

uint32_t net_client_poll(struct net_client *client, struct net_event *event) {
    uint8_t buf[HEADER + NET_PAYLOAD];
    struct net_addr from;
    const double t = net_time();
    uint32_t type;

    if (client->connecting && !client->connected &&
        t - client->last_attempt > 1.0) {
        packet_send(client->fd, &client->server, PACKET_CONNECT);
        client->last_attempt = t;
    }

    const int n = udp_recv(client->fd, &from, buf, sizeof(buf));

    if (n < 0 || !packet_check(buf, n, &type) ||
        !addr_eq(&from, &client->server)) {
        return 0;
    }

    if (type == PACKET_CONNECT_ACKNOWLEDGMENT) {
        if (client->connected) {
            return 0;
        }

        client->connected = true;
        client->connecting = false;

        client->id = n >= HEADER + 4
                         ? (uint32_t)buf[5] | ((uint32_t)buf[6] << 8) |
                               ((uint32_t)buf[7] << 16) |
                               ((uint32_t)buf[8] << 24)
                         : 0;

        *event = (struct net_event){
            .type = NET_EVENT_CONNECT,
            .client_id = client->id,
        };

        return 1;
    }

    if (type == PACKET_DISCONNECT) {
        client->connected = false;
        client->connecting = false;

        *event = (struct net_event){
            .type = NET_EVENT_DISCONNECT,
            .client_id = client->id,
        };

        return 1;
    }

    if (type == PACKET_DATA) {
        if (!client->connected) {
            return 0;
        }

        const uint32_t payload = (uint32_t)(n - HEADER);

        *event = (struct net_event){
            .type = NET_EVENT_DATA,
            .client_id = client->id,
            .len = payload,
        };
        memcpy(event->data, buf + HEADER, payload);

        return 1;
    }

    if (type == PACKET_VOICE) {
        if (!client->connected || n < HEADER + 4) {
            return 0;
        }

        const uint32_t sender = (uint32_t)buf[5] | ((uint32_t)buf[6] << 8) |
                                ((uint32_t)buf[7] << 16) |
                                ((uint32_t)buf[8] << 24);
        const struct vec2 pos = *(struct vec2 *)(buf + HEADER + 4);
        const int payload_len = n - HEADER - 4 - sizeof(pos);

        if (voice_recv_callback && payload_len > 0) {
            voice_recv_callback(sender, buf + HEADER + 4 + sizeof(pos),
                                payload_len, pos);
        }

        return 0;
    }

    return 0;
}

void net_client_send(const struct net_client *client, const void *data,
                     uint32_t len) {
    if (!client->connected) {
        return;
    }

    if (len > NET_PAYLOAD)
        len = NET_PAYLOAD;

    uint8_t buf[HEADER + NET_PAYLOAD];

    packet_pack(buf, PACKET_DATA);
    memcpy(buf + HEADER, data, len);
    udp_send(client->fd, &client->server, buf, HEADER + len);
}

void net_client_send_voice(const struct net_client *client, const void *data,
                           uint32_t len, struct vec2 pos) {
    if (!client || !client->connected) {
        return;
    }

    uint32_t new_len = len + sizeof(pos);

    if (new_len > NET_PAYLOAD) {
        new_len = NET_PAYLOAD;
    }

    uint8_t buf[HEADER + NET_PAYLOAD];
    packet_pack(buf, PACKET_VOICE);
    memcpy(buf + sizeof(pos) + HEADER, data, len);

    *(struct vec2 *)(buf + HEADER) = pos;

    udp_send(client->fd, &client->server, buf, HEADER + new_len);
}
