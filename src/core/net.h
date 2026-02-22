#ifndef NET_H
#define NET_H

#include <stdbool.h>
#include <stdint.h>

#define NET_TIMEOUT 10.0
// this is the same idea as a header, used to ignore any packages that do not belong to the game
#define NET_PROTOCOL_ID 0x696969Eu

enum {
    NET_EVENT_NONE,
    NET_EVENT_CONNECT,
    NET_EVENT_DISCONNECT,
};

struct net_addr {
    uint32_t host;
    uint16_t port;
};

struct net_event {
    uint32_t type;
    uint32_t client_id;

    uint8_t data[1400];

    uint32_t len;
};

struct net_peer {
    struct net_addr addr;
    double last_recv;
    // whether this peer is currently active or not
    bool alive;
};

struct net_server {
    int fd;

    struct net_peer *peers;
    uint32_t max_clients;
    uint32_t n;
    double last_sweep;
    uint32_t sweep_index;
};

struct net_client {
    int fd;

    struct net_addr server;

    bool connected;
    bool connecting;

    uint32_t id;

    double last_attempt;
};

// `n` - max clients
struct net_server *net_server_create(uint16_t port, uint32_t n);

void net_server_destroy(struct net_server *server);

uint32_t net_server_poll(struct net_server *server, struct net_event *event);

struct net_client *net_client_create(const char *host, uint16_t port);

void net_client_disconnect(struct net_client *client);

void net_client_destroy(struct net_client *client);

uint32_t net_client_poll(struct net_client *client, struct net_event *event);

#endif /* NET_H */
