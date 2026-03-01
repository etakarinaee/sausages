
#include "audio.h"
#include <portaudio.h>

#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>

#define FRAMES_PER_BUFFER 512

struct audio_context audio_context = {0};

static inline int check_err(PaError err) {
    if (err != paNoError) {
        fprintf(stderr, "portaudio audio_context.error: %s\n", Pa_GetErrorText(audio_context.err));
        return 1;
    }

    return 0;
}

static int audio_callback(const void* input_buf, void* out_buf, uint64_t frames_per_buf, 
                          const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags flags, void* user_data) {

    return 0;
}

static int create_stream(int dev) {
    const PaDeviceInfo *dev_info = Pa_GetDeviceInfo(dev);
    if (!dev_info) {
        fprintf(stderr, "audio: invalid device index: %d\n", dev);
        return 1;
    }

    audio_context.data.channels = (dev_info->maxInputChannels >= 2) ? 2 : 1;
    audio_context.data.sample_rate = dev_info->defaultSampleRate;

    PaStreamParameters input_param = {
        .channelCount = audio_context.data.channels,
        .device = dev,
        .hostApiSpecificStreamInfo = NULL,
        .sampleFormat = paFloat32,
        .suggestedLatency = Pa_GetDeviceInfo(dev)->defaultLowInputLatency,
    };

    audio_context.err = Pa_OpenStream(&audio_context.stream, &input_param, NULL, dev_info->defaultSampleRate,
                                      FRAMES_PER_BUFFER, paNoFlag, audio_callback, &audio_context.data);
    if (check_err(audio_context.err)) {
        fprintf(stderr, "audio: failed to open stream\n");
        return 1;
    }

    audio_context.err = Pa_StartStream(audio_context.stream);
    if (check_err(audio_context.err)) {
        fprintf(stderr, "audio: failed to start stream\n");
        return 1;
    }

    return 0;
}

int audio_init() {
    audio_context.err = Pa_Initialize();
    if (check_err(audio_context.err)) {
        fprintf(stderr, "audio: failed to init portaudio\n");
        return 1;
    }

    int dev = Pa_GetDefaultInputDevice();
    if (dev == paNoDevice) {
        fprintf(stderr, "audo: failed to find default device\n");
        return 1;
    }


    return 0;
}

