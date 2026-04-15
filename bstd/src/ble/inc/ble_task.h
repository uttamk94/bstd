#pragma once

typedef enum {
    BLE_CMD_DATA,
    BLE_CMD_MAX
} ble_cmd_t;

typedef struct {
    ble_cmd_t cmd;
    unsigned int len;
    unsigned char data[32];
} ble_msg_t;


typedef void (*ble_handler)(void *data, unsigned short len);

int set_ble_handler(ble_cmd_t cmd, ble_handler handler);
int insert_ble_msg(ble_cmd_t cmd, unsigned short len, const void *data);

int init_ble_task();
int start_ble_task();