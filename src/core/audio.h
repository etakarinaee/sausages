
#ifndef AUDIO_H
#define AUDIO_H

#include <portaudio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h> 

#include "net.h"

#define AUDIO_FRAMES_PER_BUFFER 512

    /* for max 2 channels 2x */
#define AUDIO_BUFFER_COUNT (AUDIO_FRAMES_PER_BUFFER * 2)

#define AUDIO_BUFFER_RING_COUNT (AUDIO_BUFFER_COUNT * 8)

#define AUDIO_MAGIC 0x676767Au
#define AUDIO_MAGIC_SIZE sizeof(uint32_t)

enum {
    AUDIO_INPUT_AVAILABLE,
    AUDIO_INPUT_NOT_AVAILALBE,
};

struct ring_buf {
    float buf[AUDIO_BUFFER_RING_COUNT];
    _Atomic int write_pos;
    _Atomic int read_pos;
};

typedef void (*on_chunk_ready_fn)(float *samples, int count, void* userdata);

struct audio_send_ctx {
    struct ring_buf *in;
    _Atomic bool running;

    on_chunk_ready_fn on_chunk_ready;
    void *userdata;
    int frames_per_chunk; // == AUDIO_FRAMES_PER_BUFFER
    int channels;
};

struct audio_receive_server_ctx {
    struct net_server *sp;

    _Atomic bool running;
};

struct audio_receive_client_ctx {
    struct net_client *cp;
    struct ring_buf *out;

    _Atomic bool running;
};

struct audio_data {  
    int channels_in;
    int channels_out;
    int sample_rate_in;
    int sample_rate_out;

    /* only gets written in audio callback 
       and only read in lua api */
    struct ring_buf in; /* microfon input */

    /* only gets read in audio callback 
       and only written in lua api */
    struct ring_buf out; /* output from other clients */

    _Atomic bool playback_ready;
};

struct audio_context {
    PaError err;
    PaStream *stream;

    struct audio_data data;

    pthread_t send_thread;
    struct audio_send_ctx send_ctx;

    pthread_t receive_server_thread;
    struct audio_receive_server_ctx receive_server_ctx;

    pthread_t receive_client_thread;
    struct audio_receive_client_ctx receive_client_ctx;

    on_chunk_ready_fn on_chunk_ready;
    void *on_chunk_ready_userdata;
};

extern struct audio_context audio_context;

int audio_init();
void audio_deinit();

void ring_buf_write(struct ring_buf *ring, float val);
float ring_buf_read(struct ring_buf *ring);
int ring_buf_available(struct ring_buf *ring);

void *audio_send_thread(void *arg);
void *audio_receive_server_thread(void *arg);
void *audio_receive_client_thread(void *arg);
void on_chunk_ready(float *samples, int count, void* userdata);

#endif // AUDIO_H

