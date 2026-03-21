#include "voice.h"

#ifndef SERVER

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include <opus/opus.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "net.h"

#define SAMPLE_RATE 48000
#define CHANNELS 1
#define FRAME_MS 20
#define FRAME_SIZE (SAMPLE_RATE * FRAME_MS / 1000)
#define PACKET 512
#define PEERS 32
#define CAPTURE_RING (FRAME_SIZE * 16)
#define PLAYBACK_RING (FRAME_SIZE * 16)
#define OUT_PACKETS 8

struct voice_peer {
    int16_t buffer[PLAYBACK_RING];
    uint32_t buffer_write;
    uint32_t buffer_read;

    OpusDecoder *opus_decoder;
    bool alive;
};

static struct {
    ma_device capture_device;
    ma_device playback_device;

    OpusEncoder *opus_encoder;

    // this is written by the audio thread and is read by main thread
    int16_t capture_ring[CAPTURE_RING];
    uint32_t capture_write;
    // this is main thread only!
    uint32_t capture_read;

    uint8_t output_data[OUT_PACKETS][PACKET];
    int output_lens[OUT_PACKETS];
    int output_len;

    struct voice_peer peers[PEERS];
    struct net_client *client;
    struct vec2 pos;

    bool alive;
    bool muted;
    bool ptt;
    float volume;
} voice;

static void capture_callback(ma_device *device, void *output, const void *input,
                             const ma_uint32 n) {
    (void)device;
    (void)output;

    const int16_t *samples = (const int16_t *)input;
    uint32_t write = __atomic_load_n(&voice.capture_write, __ATOMIC_RELAXED);

    for (ma_uint32 i = 0; i < n; i++) {
        voice.capture_ring[write % CAPTURE_RING] = samples[i];
        write++;
    }

    __atomic_store_n(&voice.capture_write, write, __ATOMIC_RELEASE);
}

static void playback_callback(ma_device *device, void *output,
                              const void *input, const ma_uint32 n) {
    (void)device;
    (void)input;
    (void)output;

    int16_t *destination = (int16_t *)output;
    memset(destination, 0, n * sizeof(int16_t));

    for (int peer = 0; peer < PEERS; peer++) {
        if (!voice.peers[peer].alive) {
            continue;
        }

        uint32_t read =
            __atomic_load_n(&voice.peers[peer].buffer_read, __ATOMIC_RELAXED);
        const uint32_t write =
            __atomic_load_n(&voice.peers[peer].buffer_write, __ATOMIC_ACQUIRE);

        for (ma_uint32 i = 0; i < n && read < write; i++, read++) {
            int32_t sample =
                (int32_t)destination[i] +
                (int32_t)(voice.peers[peer].buffer[read % PLAYBACK_RING] *
                          voice.volume);
            if (sample > 32767) {
                sample = 32767;
            }
            if (sample < -32767) {
                sample = -32767;
            }

            destination[i] = (int16_t)sample;
        }
        __atomic_store_n(&voice.peers[peer].buffer_read, read,
                         __ATOMIC_RELAXED);
    }
}

int voice_init(void) {
    int err;
    memset(&voice, 0, sizeof(voice));
    voice.volume = 1.f;

    OpusEncoder *opus_encoder =
        opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_VOIP, &err);

    if (err != OPUS_OK || !opus_encoder) {
        fprintf(stderr, "opus encoder: %s\n", opus_strerror(err));
        return -1;
    }
    opus_encoder_ctl(opus_encoder, OPUS_SET_BITRATE(24000));
    opus_encoder_ctl(opus_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    voice.opus_encoder = opus_encoder;

    ma_device_config capture_device =
        ma_device_config_init(ma_device_type_capture);
    capture_device.capture.format = ma_format_s16;
    capture_device.capture.channels = CHANNELS;
    capture_device.sampleRate = SAMPLE_RATE;
    capture_device.dataCallback = capture_callback;
    capture_device.periodSizeInFrames = FRAME_SIZE;

    if (ma_device_init(NULL, &capture_device, &voice.capture_device) !=
        MA_SUCCESS) {
        fprintf(stderr, "capture device\n");
        opus_encoder_destroy(opus_encoder);

        return -1;
    }

    ma_device_config playback_device =
        ma_device_config_init(ma_device_type_playback);
    playback_device.playback.format = ma_format_s16;
    playback_device.playback.channels = CHANNELS;
    playback_device.sampleRate = SAMPLE_RATE;
    playback_device.dataCallback = playback_callback;
    playback_device.periodSizeInFrames = FRAME_SIZE;

    if (ma_device_init(NULL, &playback_device, &voice.playback_device) !=
        MA_SUCCESS) {
        fprintf(stderr, "playback device\n");
        ma_device_uninit(&voice.capture_device);
        opus_encoder_destroy(opus_encoder);

        return -1;
    }

    if (ma_device_start(&voice.capture_device) != MA_SUCCESS ||
        ma_device_start(&voice.playback_device) != MA_SUCCESS) {
        fprintf(stderr, "start audio device\n");
        ma_device_uninit(&voice.capture_device);
        ma_device_uninit(&voice.playback_device);
        opus_encoder_destroy(opus_encoder);

        return -1;
    }

    voice.alive = true;

    return 0;
}

void voice_quit(void) {
    if (!voice.alive) {
        return;
    }

    ma_device_uninit(&voice.capture_device);
    ma_device_uninit(&voice.playback_device);
    opus_encoder_destroy(voice.opus_encoder);

    for (int i = 0; i < PEERS; i++) {
        if (voice.peers[i].opus_decoder) {
            opus_decoder_destroy(voice.peers[i].opus_decoder);
        }
    }

    voice.alive = false;
}

int voice_update(void) {
    if (!voice.alive) {
        return 0;
    }
    voice.output_len = 0;

    math_vec2_print(voice.pos);

    const bool should_transmit = !voice.muted && voice.ptt;
    if (!should_transmit) {
        return 0;
    }

    const uint32_t write =
        __atomic_load_n(&voice.capture_write, __ATOMIC_ACQUIRE);

    // the thread falls behind
    if (write - voice.capture_read > CAPTURE_RING) {
        voice.capture_read = write - FRAME_SIZE;
    }

    while (write - voice.capture_read >= (uint32_t)FRAME_SIZE &&
           voice.output_len < OUT_PACKETS) {
        int16_t frame[FRAME_SIZE];

        for (int i = 0; i < FRAME_SIZE; i++) {
            frame[i] =
                voice.capture_ring[(voice.capture_read + i) % CAPTURE_RING];
        }

        voice.capture_read += FRAME_SIZE;

        const int n = opus_encode(voice.opus_encoder, frame, FRAME_SIZE,
                                  voice.output_data[voice.output_len], PACKET);
        if (n > 0) {
            voice.output_lens[voice.output_len] = n;
            voice.output_len++;
        }
    }

    if (voice.client) {
        for (int i = 0; i < voice.output_len; i++) {
            net_client_send_voice(voice.client, voice.output_data[i],
                                  (uint32_t)voice.output_lens[i], voice.pos);
        }
    }

    return voice.output_len;
}

void voice_receive(const uint32_t peer_id, const uint8_t *data, const int len,
                   struct vec2 pos) {
    if (!voice.alive || peer_id >= PEERS || len <= 0) {
        return;
    }

    struct voice_peer *peer = &voice.peers[peer_id];

    if (!peer->opus_decoder) {
        int err;

        peer->opus_decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
        if (err != OPUS_OK || !peer->opus_decoder) {
            fprintf(stderr, "decoder (peer %u)", peer_id);
            peer->opus_decoder = NULL;

            return;
        }

        __atomic_store_n(&peer->buffer_write, 0u, __ATOMIC_SEQ_CST);
        __atomic_store_n(&peer->buffer_read, 0u, __ATOMIC_SEQ_CST);

        peer->alive = true;
    }

    int16_t decoded[FRAME_SIZE];
    const int samples =
        opus_decode(peer->opus_decoder, data, len, decoded, FRAME_SIZE, 0);
    if (samples <= 0) {
        return;
    }

    uint32_t write = __atomic_load_n(&peer->buffer_write, __ATOMIC_RELAXED);

    float distance = math_vec2_distance(voice.pos, pos);

    for (int i = 0; i < samples; i++) {
        peer->buffer[write % PLAYBACK_RING] = decoded[i] * (1 / distance);
        write++;
    }

    __atomic_store_n(&peer->buffer_write, write, __ATOMIC_RELEASE);
}

void voice_peer_remove(uint32_t peer_id) {
    if (peer_id >= PEERS) {
        return;
    }

    struct voice_peer *peer = &voice.peers[peer_id];
    peer->alive = false;

    if (peer->opus_decoder) {
        opus_decoder_destroy(peer->opus_decoder);
        peer->opus_decoder = NULL;
    }
}

void voice_set_muted(bool muted) { voice.muted = muted; }

void voice_set_volume(float volume) { voice.volume = volume; }

void voice_set_ptt_active(bool active) { voice.ptt = active; }

void voice_set_client(struct net_client *client) { voice.client = client; }

void voice_set_pos(struct vec2 pos) { voice.pos = pos; }

#else

int voice_init(void) { return 0; }

void voice_quit(void) {}

int voice_update(void) { return 0; }

void voice_receive(uint32_t peer_id, const uint8_t *data, int len,
                   struct vec2 pos) {
    (void)peer_id;
    (void)data;
    (void)len;
    (void)pos;
}

void voice_peer_remove(uint32_t peer_id) { (void)peer_id; }

void voice_set_muted(bool muted) { (void)muted; }

void voice_set_volume(float volume) { (void)volume; }

void voice_set_ptt_active(bool active) { (void)active; }

void voice_set_client(struct net_client *client) { (void)client; }

void voice_set_pos(struct vec2 pos) { (void)pos; }

#endif
