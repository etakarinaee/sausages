#ifndef VOICE_H
#define VOICE_H

#include "cmath.h"
#include <stdbool.h>
#include <stdint.h>

struct net_client;

int voice_init(void);

void voice_quit(void);

int voice_update(void);

void voice_receive(uint32_t peer_id, const uint8_t *data, int len,
                   struct vec2 pos);

void voice_peer_remove(uint32_t peer_id);

void voice_set_muted(bool muted);

void voice_set_volume(float volume);

void voice_set_ptt_active(bool active);

void voice_set_client(struct net_client *client);

void voice_set_pos(struct vec2 pos);

#endif // VOICE_H
