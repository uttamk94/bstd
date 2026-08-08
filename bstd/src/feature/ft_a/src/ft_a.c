#include <stdio.h>
#include "ft_a.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "sensor.h"
#include "shandler.h"
#include "loggers.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

#if defined(CONFIG_BLE_ENABLE)
#include "ble.h"
#endif

#if defined(CONFIG_ENCODER_ENABLE)
#include "my_encoder.h"
#endif

#if defined(CONFIG_LCR_ENABLE)
#include "pattern_sub.h"
#endif

#if defined(CONFIG_WEARABLE_COMPRESSION)
#include "compression.h"
#include "codebook.h"
#endif

#define MINITUE_COUNT 5
#define ACC_COUNT 8
#define RRI_COUNT 8

typedef struct __packed {
    unsigned char indx;
    unsigned short rri;
} rri_t;

typedef struct __packed {
    int acc[MINITUE_COUNT][ACC_COUNT];
    rri_t rri[MINITUE_COUNT][RRI_COUNT];
} sleep_raw_t;
int x = sizeof(sleep_raw_t);

void populate(sleep_raw_t *raw) {
    for (int i = 0; i < MINITUE_COUNT; i++) {
        for (int j = 0; j < ACC_COUNT; j++) {
            raw->acc[i][j] = 58392 + rand() % 99999;
        }
    }
    for (int i = 0; i < MINITUE_COUNT; i++) {
        for (int j = 0; j < RRI_COUNT; j++) {
            raw->rri[i][j].indx = rand() % 255;
            raw->rri[i][j].rri = rand() % 65535;
        }
    }
}

void print_sleep_raw(const sleep_raw_t *raw) {
    for (int i = 0; i < MINITUE_COUNT; i++) {
        for (int j = 0; j < ACC_COUNT; j++) {
            log_i("%d ", raw->acc[i][j]);
        }
        log_i("");
    }
    for (int i = 0; i < MINITUE_COUNT; i++) {
        for (int j = 0; j < RRI_COUNT; j++) {
            log_i("(%u, %u) ", raw->rri[i][j].indx, raw->rri[i][j].rri);
        }
        log_i("");
    }
}


void compare_sleep_raw(const sleep_raw_t *raw, sleep_raw_t *metrics) {
    // Placeholder for sleep metrics computation logic
    for (int i = 0; i < MINITUE_COUNT; i++) {
        for (int j = 0; j < ACC_COUNT; j++) {
            if (metrics->acc[i][j] != raw->acc[i][j]) {
                log_e("acc not matched");
                return;
            }
        }
        for (int j = 0; j < RRI_COUNT; j++) {
            if (metrics->rri[i][j].indx != raw->rri[i][j].indx ||
                metrics->rri[i][j].rri != raw->rri[i][j].rri) {
                log_e("rri not matched");
                return;
            }
        }
    }
    log_i("matched");
}

unsigned int count = 0;
void on_sens_data_received(unsigned char type, unsigned int len, void *data) {
    log_i("sens_data");
    log_i("%u, %u,", type, len);

#if defined(CONFIG_ENCODER_ENABLE)
    log_w("%d", encode(2, 4));
#endif

    uint8_t output[4096];
    sleep_raw_t input = {0, };
    populate(&input);

#if defined(CONFIG_LCR_ENABLE)
    int enc_len = encode_buf_bound(sizeof(input));
    if (enc_len <= 0 || enc_len > sizeof(output)) {
        log_e("Invalid encode buffer size: %d", enc_len);
        return;
    }
    
    int encoded_len = encode(&input, sizeof(input), output);
    log_i("encoded_len: %d", encoded_len);
    if (encoded_len > 0) {
        sleep_raw_t decoded = {0, };

        int decoded_len = decode(output, encoded_len, &decoded);
        log_i("decoded_len: %d", decoded_len);
        if (decoded_len > 0) {
            compare_sleep_raw(&input, &decoded);
        }
    }
#endif

# if defined(CONFIG_WEARABLE_COMPRESSION)
    /* Use the user-friendly byte-oriented wrappers. They accept a raw
     * byte buffer (interleaved int16 [sample][channel]) and a byte length,
     * so the caller does not need to manage int16 sample counts.
     *
     * IMPORTANT: The previous test used RANDOM data (58392 + rand()%99999).
     * Random data is incompressible — it never matches the learned codebook,
     * so every window falls back to RAW storage and the TLV container adds
     * overhead, making the "compressed" output LARGER than the input.
     * For a meaningful compression test we must feed CORRELATED data
     * (e.g. smooth sine waves) that the codebook can actually match.
     */
    const uint16_t test_samples = CONFIG_COMPRESSION_WINDOW_SIZE;
    const uint16_t test_channels = CONFIG_COMPRESSION_NUM_CHANNELS;
    const uint16_t total_elems = test_samples * test_channels;

    int16_t data_in[CONFIG_COMPRESSION_WINDOW_SIZE * CONFIG_COMPRESSION_NUM_CHANNELS] = {0};
    int16_t data_out[CONFIG_COMPRESSION_WINDOW_SIZE * CONFIG_COMPRESSION_NUM_CHANNELS] = {0};

    /* Generate data that ACTUALLY matches the learned codebook.
     *
     * wzip is a pattern-dictionary compressor: it only achieves compression
     * when the input resembles the codebook patterns (learned from real
     * sensor signals with amplitudes up to ±30000). A synthetic sine wave
     * with tiny amplitude (500-1500) does NOT match any codebook pattern,
     * so every window falls back to RAW storage and the TLV overhead makes
     * the output LARGER than the input (ratio < 1.0).
     *
     * To demonstrate real compression, each channel is built from an actual
     * codebook pattern of the matching channel type, plus small noise.
     * The matcher then finds a near-perfect match, residuals are tiny, and
     * Golomb-Rice coding compresses them well. */
    for (uint16_t ch = 0; ch < test_channels; ch++) {
        /* Find the first codebook pattern belonging to this channel type */
        uint16_t pat_idx = 0;
        for (uint16_t i = 0; i < CODEBOOK_SIZE; i++) {
            if (codebook_types[i] == ch) {
                pat_idx = i;
                break;
            }
        }
        /* Use the pattern + small noise (±3) as this channel's data */
        for (uint16_t s = 0; s < test_samples; s++) {
            int16_t noise = (int16_t)((rand() % 7) - 3);
            data_in[s * test_channels + ch] = codebook[pat_idx][s] + noise;
        }
    }

    /* Compress using the byte-oriented wrapper (byte length, not sample count) */
    uint16_t in_bytes = total_elems * sizeof(int16_t);
    uint16_t wlen = sizeof(output);
    int ret = wzip_compress_bytes(data_in, in_bytes, output, &wlen);
    log_w("wzip_compress_bytes ret: %d, in_bytes: %u out_bytes: %u (ratio %.2f)",
          ret, in_bytes, wlen, (wlen > 0) ? (double)in_bytes / wlen : 0.0);

    /* Decompress using the byte-oriented wrapper */
    uint16_t out_bytes = sizeof(data_out);
    ret = wzip_decompress_bytes(output, wlen, data_out, &out_bytes);
    log_w("wzip_decompress_bytes ret: %d, out_bytes: %u wlen: %u", ret, out_bytes, wlen);

    if (ret == WZIP_OK && out_bytes == in_bytes) {
        /* wzip_decompress() emits channels as contiguous blocks [ch][sample],
         * while the input format is interleaved [sample][ch].
         * Re-interleave to verify round-trip integrity. */
        bool matched = true;
        for (uint16_t ch = 0; ch < test_channels && matched; ch++) {
            for (uint16_t s = 0; s < test_samples; s++) {
                if (data_out[ch * test_samples + s] != data_in[s * test_channels + ch]) {
                    matched = false;
                    break;
                }
            }
        }
        if (matched) {
            log_i("wzip round-trip matched");
        } else {
            log_e("wzip round-trip mismatch");
        }
    }

#endif
    count++;
#if defined(CONFIG_BLE_ENABLE)
    //ble_log((char *)&count, sizeof(count));
#endif
}

sns_handler_t ft_a_handler = {
    .cid = CID_FA,
    .type = SENS_TYPE_S,
    .sensor_data_cb = on_sens_data_received,
    .sensor_status = NULL,
};

int init_ft_a(void) {
    log_i("InitA");
    return 0;
}

int start_ft_a(void) {
    log_i("Start A");
    int ret = add_sensor(&ft_a_handler);
    if (ret != true) {
        log_e("Failed to register feature A handler");
        return -1;
    }
    log_i("Feature A started");
    return 0;
}

int stop_ft_a(void) {
    log_i("Stop A");
    del_sensor(&ft_a_handler);
    return 0;
}