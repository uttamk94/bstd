#include "data_commu.h"
#include "loggers.h"
#include "ble.h"

typedef enum {
    MSG_A,
    MSG_B,
    MSG_C,
    MSG_D,
    MSG_MAX
} msg_id_t;

typedef struct {
    msg_id_t id;
    void (*msg_handler)(void *buff, unsigned short len);
} commu_msg_t;

commu_msg_t msg_table[] = {
    {MSG_A, NULL},
    {MSG_B, NULL},
    {MSG_C, NULL},
    {MSG_D, NULL},
};

int send_commu_data(void *data, unsigned short len) {
    return notify_value(data, len);
}

void on_data_received(unsigned short len, void *data) {
    log_i("on_data_received");
    if (!data || len < 1) {
        log_e("data error");
        return;
    }
    uint8_t *buff = (uint8_t *) data;
    if (msg_table[*buff].msg_handler) {
        msg_table[*buff].msg_handler(data, len);
    }
}

int init_data_commu() {

}

int start_data_commu() {
    set_ble_handler(BLE_CMD_DATA, on_data_received);
}