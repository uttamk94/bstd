#include "data_commu.h"
#include "loggers.h"
#include "ble.h"

client_handler_t client_table[CLIENT_MAX];

void set_client_handler(client_id_t client, msg_handler handler) {
    if (client >= CLIENT_MAX) {
        log_e("Invalid client id");
        return;
    }
    client_table[client].id = client;
    client_table[client].handler = handler;
}

int send_commu_data(void *data, unsigned short len) {
    return notify_value(data, len);
}

void on_data_received( void *buf, unsigned short len) {
    log_i("on_data_received");
    if (!buf || len < 1) {
        log_e("data error");
        return;
    }
    uint8_t *data = (uint8_t *) buf;
    client_id_t client = data[0];
    log_i("client: %u", client);
    if (client < CLIENT_MAX && client_table[client].handler) {
        client_table[client].handler(data + 1, len - 1);
    }
}

int init_data_commu() {
    memset(client_table, 0, sizeof(client_table));
    return 0;
}

int start_data_commu() {
    set_ble_handler(BLE_CMD_DATA, on_data_received);
    return 0;
}