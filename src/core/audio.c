
#include "audio.h"
#include <bits/time.h>
#include <portaudio.h>

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdint.h> 
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* idk maybe not hardcode */
#define SAMPLE_RATE 48000.0

struct audio_context audio_context = {0};

void ring_buf_write(struct ring_buf *ring, float val) {
    int wp = atomic_load_explicit(&ring->write_pos, memory_order_relaxed);
    int rp = atomic_load_explicit(&ring->read_pos, memory_order_acquire);

    int available = AUDIO_BUFFER_RING_COUNT - (wp - rp);
    if (available <= 0) return;
    
    ring->buf[wp % AUDIO_BUFFER_RING_COUNT] = val;

    atomic_store_explicit(&ring->write_pos, wp + 1, memory_order_release);
}

float ring_buf_read(struct ring_buf *ring) {
    int rp = atomic_load_explicit(&ring->read_pos, memory_order_relaxed);
    int wp = atomic_load_explicit(&ring->write_pos, memory_order_acquire);

    if (rp == wp) return 0.0f;

    float val = ring->buf[rp % AUDIO_BUFFER_RING_COUNT];
    
    atomic_store_explicit(&ring->read_pos, rp + 1, memory_order_release);

    return val;
}

static inline int check_err(PaError err) {
    if (err != paNoError) {
        fprintf(stderr, "portaudio audio_context.error: %s\n", Pa_GetErrorText(audio_context.err));
        return 1;
    }

    return 0;
}

int ring_buf_available(struct ring_buf *ring) {
    int wp = atomic_load_explicit(&ring->write_pos, memory_order_acquire);
    int rp = atomic_load_explicit(&ring->read_pos, memory_order_relaxed);
    return wp - rp;
}

static void *audio_send_thread(void *arg) {
    struct audio_send_ctx *ctx = (struct audio_send_ctx*)arg;
    int chunk_size = ctx->frames_per_chunk * ctx->channels;

    float *chunk = malloc(chunk_size * sizeof(float));

    /* interval in ns (frames / sample_rate) */
    long interval_ns = (long)((ctx->frames_per_chunk / SAMPLE_RATE) * 1e9);

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (atomic_load_explicit(&ctx->running, memory_order_relaxed)) {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

        next.tv_nsec += interval_ns;
        if (next.tv_nsec >= 1000000000L) {
            next.tv_nsec -= 1000000000L;
            next.tv_sec++;
        }

        if (ring_buf_available(ctx->in) >= chunk_size) {
            for (int i = 0; i < chunk_size; i++) {
                chunk[i] = ring_buf_read(ctx->in);
            }
            ctx->on_chunk_ready(chunk, chunk_size, ctx->userdata);
        }
    }

    free(chunk);
    return NULL;
}

static int audio_callback(const void* input_buf, void* out_buf, unsigned long frames_per_buf, 
                          const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags flags, void* user_data) {
    (void)time_info; (void)flags;
    struct audio_data* data = user_data;
    float* out = (float*)out_buf;

    /* write mic input to ring buffer for send thread to pick it up */
    if (input_buf) {
        float* in = (float*)input_buf;

        for (int i = 0; i < frames_per_buf * data->channels_in; i++) {
            static float phase = 0.0f;
            ring_buf_write(&data->in, 0.1f * sin(phase));
            phase += 2.0f * 3.14159265359 * 440.0f / 48000.0f;
        }
    }

    /* Output */
    for (unsigned long i = 0; i < frames_per_buf; i++) {
        float sample = ring_buf_read(&data->out);

        if (data->channels_out == 1) {
            out[i] = sample;
        }
        else {
            out[i * 2] = sample;
            out[i * 2 + 1] = sample;
        }
    }

    return paContinue;
}

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
    param.suggestedLatency = input ? dev_info->defaultLowInputLatency : dev_info->defaultLowOutputLatency;

    return param;
}

static int create_stream(int dev_input, int dev_output) {
 
    PaStreamParameters input_param = get_dev_info(dev_input, &audio_context.data.sample_rate_in, true);
    audio_context.data.channels_in = input_param.channelCount;

    /* TODO: maybe  also save sample rate */
    PaStreamParameters output_param = get_dev_info(dev_output, &audio_context.data.sample_rate_out, false);
    audio_context.data.channels_out = output_param.channelCount;

    /* TODO: maybe not hardcode sample rate */
    audio_context.err = Pa_OpenStream(&audio_context.stream, &input_param, &output_param, SAMPLE_RATE,
                                      AUDIO_FRAMES_PER_BUFFER, paClipOff, audio_callback, &audio_context.data);
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
    memset(&audio_context.data, 0, sizeof(struct audio_data));

    audio_context.err = Pa_Initialize();
    if (check_err(audio_context.err)) {
        fprintf(stderr, "audio: failed to init portaudio\n");
        return 1;
    }

    printf("Host Audio API: %s\n", Pa_GetHostApiInfo(Pa_GetDefaultHostApi())->name);

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

    audio_context.send_ctx = (struct audio_send_ctx){
        .in = &audio_context.data.in,
        .frames_per_chunk = AUDIO_FRAMES_PER_BUFFER,
        .channels = audio_context.data.channels_in,
        .on_chunk_ready = audio_context.on_chunk_ready,
        .userdata = audio_context.on_chunk_ready_userdata,
    };

    atomic_store(&audio_context.send_thread_running, true);
    atomic_store(&audio_context.send_ctx.running, true);
    pthread_create(&audio_context.send_thread, NULL, audio_send_thread, &audio_context.send_ctx);


    return 0;
}

void audio_deinit() {
    atomic_store(&audio_context.send_ctx.running, false);
    pthread_join(audio_context.send_thread, NULL);

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

