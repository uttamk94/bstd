#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include "sensor.h"
#include "loggers.h"

#define MAX_CLNT    0x10

typedef struct {
    int index;
    sensor_data_cb handler[MAX_CLNT];
} sens_table_t;

sens_table_t table[SENS_MAX];

int reg_sensor(sens_type_t type, sensor_data_cb handler) {
    if (type >= SENS_MAX || !handler) {
        log_e("Inv type=%d, handler=%p", type, handler);
        return -EINVAL;
    }
    if (table[type].index >= MAX_CLNT) {
        log_e("No slots for sensor type %d", type);
        return -ENOSPC;
    }
    table[type].handler[table[type].index++] = handler;
    table[type].index = table[type].index % MAX_CLNT;
    log_i("Registered sensor type %d", type);
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

/* Core dispatcher: called by sensor sources (e.g. sensor_gpio.c)
 * to publish data to all registered consumers.
 */
int insert_sensor_data(sens_type_t type, unsigned int len, void *data) {
    if (type >= SENS_MAX || !data || len == 0) {
        log_e("Invalid sensor data: type=%d, len=%d", type, len);
        return -EINVAL;
    }

    for (int i = 0; i < MAX_CLNT; i++) {
        if (table[type].handler[i]) {
            table[type].handler[i](type, len, data);
        }
    }
    return 0;
}

int init_sensor(void) {
    log_i("Init");
    memset(table, 0, sizeof(table));
    return init_sensor_gpio();
}

int start_sensor(void) {
    log_i("Start");
    return start_sensor_gpio();
}

int stop_sensor(void) {
    log_i("Stop");
    return stop_sensor_gpio();
}