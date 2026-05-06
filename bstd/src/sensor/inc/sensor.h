#pragma once

typedef enum {
    SENS_TYPE_NONE = 0,
    SENS_TYPE_S = 1,
    SENS_MAX
} sens_type_t;

typedef struct {
    unsigned char flag;
    unsigned char type;
    unsigned int timestamp;
    unsigned int length;
    unsigned char data[512];
} sensor_out_t;

typedef void (*sensor_data_cb)(sens_type_t type, unsigned int len, void *data);

int reg_sensor(sens_type_t type, sensor_data_cb handler);
int unreg_sensor(sens_type_t type, sensor_data_cb handler);
int insert_sensor_data(sens_type_t type, unsigned int len, void *data);

int init_sensor();
int start_sensor();