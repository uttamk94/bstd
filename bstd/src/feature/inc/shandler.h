#pragma once

typedef enum {
    CID_FA,
    CID_FB,
    CID_MAX
} cid_t;

typedef struct {
    unsigned char cid;
    unsigned char type;
    void (*sensor_data_cb)(unsigned char type, unsigned int len, void *data);
    void (*sensor_status)(unsigned char status, unsigned char reason);
} sns_handler_t;

int add_sensor(sns_handler_t *handler);
int del_sensor(sns_handler_t *handler);
int add_all_sensor();
int del_all_sensor();

int init_shandler();
int start_shandler();