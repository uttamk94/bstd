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

#if defined(CONFIG_BLE_ENABLE)
#include "ble.h"
#endif

#if defined(CONFIG_ENCODER_ENABLE)
#include "my_encoder.h"
#endif

#if defined(CONFIG_LCR_ENABLE)
#include "pattern_sub.h"
#endif


#define MINITUE_COUNT 5
#define ACC_COUNT 8
#define RRI_COUNT 88

typedef struct __packed {
    unsigned char indx;
    unsigned short rri;
} rri_t;

typedef struct __packed {
    int acc[MINITUE_COUNT][ACC_COUNT];
    rri_t rri[MINITUE_COUNT][RRI_COUNT];
} sleep_raw_t;


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

#if defined(CONFIG_LCR_ENABLE)
    static uint8_t output[4096];
    sleep_raw_t input = {0, };
    populate(&input);
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