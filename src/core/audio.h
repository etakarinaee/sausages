
#ifndef AUDIO_H
#define AUDIO_H

#include <portaudio.h>

/* maybe for windows diffrent later */
/* also makes clear this is atomic */
#define atomic_int int

#define AUDIO_FRAMES_PER_BUFFER 512

/* for max 2 channels 2x */
#define AUDIO_BUFFER_COUNT (AUDIO_FRAMES_PER_BUFFER * 2)

#define AUDIO_BUFFER_RING_COUNT (AUDIO_BUFFER_COUNT * 8)

enum {
    AUDIO_INPUT_AVAILABLE,
    AUDIO_INPUT_NOT_AVAILALBE,
};

struct ring_buf {
    float buf[AUDIO_BUFFER_RING_COUNT];
    int write_pos;
    int read_pos;
};

struct audio_data {  
    atomic_int channels_in;
    atomic_int channels_out;
    atomic_int sample_rate_in;
    atomic_int sample_rate_out;

    /* only gets written in audio callback 
       and only read in lua api */
    struct ring_buf in; /* microfon input */

    /* only gets read in audio callback 
       and only written in lua api */
    struct ring_buf out; /* output from other clients */
};

struct audio_context {
    PaError err;
    PaStream *stream;

    struct audio_data data;
};

extern struct audio_context audio_context;

int audio_init();
void audio_deinit();

void ring_buf_write(struct ring_buf *ring, float val);
float ring_buf_read(struct ring_buf *ring);

#endif // AUDIO_H
