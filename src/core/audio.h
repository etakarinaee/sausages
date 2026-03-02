
#ifndef AUDIO_H
#define AUDIO_H

#include <portaudio.h>

/* maybe for windows diffrent later */
/* also makes clear this is atomic */
#define atomic_int int

#define AUDIO_FRAMES_PER_BUFFER 512

/* for max 2 channels 2x */
#define AUDIO_BUFFER_COUNT AUDIO_FRAMES_PER_BUFFER * 2

enum {
    AUDIO_INPUT_AVAILABLE,
    AUDIO_INPUT_NOT_AVAILALBE,
};

struct audio_data {  
    atomic_int vol_l;
    atomic_int vol_r;

    atomic_int channels_in;
    atomic_int channels_out;
    atomic_int sample_rate_in;
    atomic_int sample_rate_out;

    /*
        buffer[0] always indicates if we want to play any data 
        if data then buffer[0] == AUDIO_INPUT_AVAILABLE 
    */
    float buffer_out[AUDIO_BUFFER_COUNT]; 
    float buffer_in[AUDIO_BUFFER_COUNT];
};

struct audio_context {
    PaError err;
    PaStream *stream;

    struct audio_data data;
};

extern struct audio_context audio_context;

int audio_init();
void audio_deinit();

#endif // AUDIO_H
