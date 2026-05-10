#include "data_commu.h"
#include "loggers.h"

#if defined(CONFIG_BLE_ENABLE)
#include "ble.h"
#endif

mmsg_handler_t *client_table[CLIENT_MAX];

void set_client_handler(mmsg_handler_t *handler) {
    if (!handler || handler->id >= CLIENT_MAX) {
        log_e("Invalid handler");
        return;
    }
    client_table[handler->id] = handler;
}

int send_commu_data(void *data, unsigned short len) {
#if defined(CONFIG_BLE_ENABLE)
    return notify_value(data, len);
#endif 
    return 0;
}

void on_data_received(void *buf, unsigned short len) {
    log_i("on_data_received");
    if (!buf || len < 2) {
        log_e("data error");
        return;
    }
    unsigned char *data = (unsigned char *) buf;
    client_id_t client = data[0];
    message_t msg_type = data[1];
    if (client >= CLIENT_MAX || msg_type >= MSG_MAX) {
        log_e("client error");
        return;
    }

    mmsg_handler_t *handler = client_table[client];
    for (int i = 0; i < handler->count; i++) {
        msg_cb_t *cb = &handler->cbs[i];
        if (cb->id == msg_type) {
            cb->msg_cb(buf, len);
            return;
        }
    }
}

int init_data_commu() {
    memset(client_table, 0, sizeof(client_table));
    return 0;
}

int start_data_commu() {
#if defined(CONFIG_BLE_ENABLE)
    set_ble_handler(BLE_CMD_DATA, on_data_received);
#endif
    return 0;
}