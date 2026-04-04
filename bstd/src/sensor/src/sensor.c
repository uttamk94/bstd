#include <zephyr/kernel.h>
#include <stdio.h>
#include "sensor.h"
#include <time.h>

#define SENSOR_TH_STK_SIZE 1024 * 2
#define MAX_CLNT    0x10

typedef struct {
    int index;
    sensor_data_cb handler[MAX_CLNT];
} sens_table_t;

sens_table_t table[SENS_MAX];

int reg_sensor(sens_type_t type, sensor_data_cb handler) {
    table[type].handler[table[type].index++] = handler;
    table[type].index = table[type].index % MAX_CLNT;
    return 0;
}

int unreg_sensor(sens_type_t type, sensor_data_cb handler) {
    for (int i = 0; i < MAX_CLNT; i++) {
        if (table[type].handler[i] == handler) {
            table[type].handler[i] = NULL;
            return 0;
        }
    }
    return -1;
}

int insert_sensor_data(sens_type_t type, unsigned int len, void *data) {
    return 0;
}

void notify_sensor_data(sens_type_t type, sensor_out_t *out, unsigned int len) {
    for (int i = 0; i < MAX_CLNT; i++) {
        if (table[type].handler[i]) {
            table[type].handler[i](type, len, (unsigned char *) out);
        }
    }
}
void generate_data(sens_type_t type) {
    sensor_out_t out = {0, };
    out.type = type;
    out.length = rand() % 512;
    out.timestamp = time(NULL);
    for (int i = 0; i < out.length; i++) {
        out.data[i] = rand() % 256;
    }
    notify_sensor_data(type, &out, sizeof(sensor_out_t));
}

void sensor_thread_cb(void *arg1, void *arg2, void *arg3) {
    printf("sensor_thread\n");
    int count = 0;
    while (1) {
        printf("%s: %d\n", __func__, ++count);
        count = count % 99999;
        generate_data((sens_type_t) (rand() % SENS_MAX));
        k_msleep(1000);
    }
}

K_THREAD_DEFINE(sensor_thread, SENSOR_TH_STK_SIZE, sensor_thread_cb, NULL, NULL, NULL, 12, 0, 0);


int init_sensor() {
    printf("init_sensor\n");
    return 0;
}

int start_sensor() {
    printf("start_sensor\n");
    return 0;
}