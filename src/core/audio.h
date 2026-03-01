
#ifndef AUDIO_H
#define AUDIO_H

#include <portaudio.h>

/* maybe for windows diffrent later */
/* also makes clear this is atomic */
#define atomic_int int

struct audio_data {  
    atomic_int vol_l;
    atomic_int vol_r;

    atomic_int channels_in;
    atomic_int channels_out;
    atomic_int sample_rate;
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
