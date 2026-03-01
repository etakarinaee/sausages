
#include "audio.h"
#include <portaudio.h>

#include <stdio.h>
#include <stdint.h> 
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

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

    (void)input_buf;
    (void)time_info;
    (void)flags;

    struct audio_data* data = user_data;

    float* out = (float*)out_buf;

    static float phase = 0.0f;

    const float freq = 440.0f;
    const float amp  = 0.2f;
    const float sr   = (float)data->sample_rate;

    for (uint64_t i = 0; i < frames_per_buf; i++) {

        float sample = amp * sinf(phase);
        phase += 2.0f * (float)M_PI * freq / sr;

        if (phase >= 2.0f * (float)M_PI)
            phase -= 2.0f * (float)M_PI;

        if (data->channels_out == 2) {
            out[i * 2]     = sample;
            out[i * 2 + 1] = sample;
        } else {
            out[i] = sample;
        }
    }

    return paContinue;}

static inline PaStreamParameters get_dev_info(int dev, int *sample_rate, bool input) {
    PaStreamParameters param = {0};

    const PaDeviceInfo *dev_info = Pa_GetDeviceInfo(dev);
    if (!dev_info) {
        fprintf(stderr, "audio: invalid device index: %d\n", dev);
        return param;
    }

    if (sample_rate) *sample_rate = dev_info->defaultSampleRate;

    param.device = dev;
    param.channelCount = ((input ? dev_info->maxInputChannels : dev_info->maxOutputChannels) >= 2) ? 2 : 1;
    param.hostApiSpecificStreamInfo = NULL;
    param.sampleFormat = paFloat32;
    param.suggestedLatency = dev_info->defaultLowInputLatency;

    return param;
}

static int create_stream(int dev_input, int dev_output) {
 
    PaStreamParameters input_param = get_dev_info(dev_input, &audio_context.data.sample_rate, true);
    audio_context.data.channels_in = input_param.channelCount;

    /* TODO: maybe  also save sample rate */
    PaStreamParameters output_param = get_dev_info(dev_output, NULL, false);
    audio_context.data.channels_out = output_param.channelCount;

    audio_context.err = Pa_OpenStream(&audio_context.stream, &input_param, &output_param, audio_context.data.sample_rate,
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

    int dev_input = Pa_GetDefaultInputDevice();
    if (dev_input == paNoDevice) {
        fprintf(stderr, "audo: failed to find default input device\n");
        return 1;
    }

    int dev_output = Pa_GetDefaultOutputDevice();
    if (dev_output == paNoDevice) {
        fprintf(stderr, "audio: failed to find default output device\n");
        return 1;
    }

    create_stream(dev_input, dev_output);

    return 0;
}

void audio_deinit() {
    audio_context.err = Pa_StopStream(audio_context.stream);
    if (check_err(audio_context.err)) {
        fprintf(stderr, "audio: failed stopping stream\n");
    }

    audio_context.err = Pa_CloseStream(audio_context.stream);
    if (check_err(audio_context.err)) {
        fprintf(stderr, "audio: failed closing stream\n");
    }

    audio_context.err = Pa_Terminate();
    if (check_err(audio_context.err)) {
        fprintf(stderr, "audio: failed terminating portaudio\n");
    }
}

