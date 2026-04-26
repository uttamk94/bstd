#include <zephyr/kernel.h>

#include "ble_task.h"
#include "loggers.h"
#include "stdlib.h"

#define MSGQ_LENGTH     0x10
#define BLE_TASK_SIZE   1024 * 2

void ble_task_cb(void *p1, void *p2, void *p3);

K_MSGQ_DEFINE(ble_msgq, sizeof(ble_msg_t), MSGQ_LENGTH, 4);
K_THREAD_DEFINE(ble_task, BLE_TASK_SIZE, ble_task_cb, NULL, NULL, NULL, 0, 0, 0);

ble_handler ble_handlers[BLE_CMD_MAX];

int set_ble_handler(ble_cmd_t cmd, ble_handler handler) {
    if (cmd >= BLE_CMD_MAX) return -1;
    ble_handlers[cmd] = handler;
    return 0;
}

int insert_ble_msg(ble_cmd_t cmd, unsigned short len, const void *data) {
    ble_msg_t msg = {0, };
    msg.cmd = cmd;
    msg.len = len;
    memcpy(msg.data, data, len);
    int ret = k_msgq_put(&ble_msgq, &msg, K_NO_WAIT);
    if (ret) {
        log_e("insert failed");
    }
    return ret;
}

void ble_task_cb(void *p1, void *p2, void *p3) {
    log_i("started");
    ble_msg_t msg = {0, };
    while (1) {
        if (!k_msgq_get(&ble_msgq, &msg, K_FOREVER)) {
            log_i("cmd %d, len: %d, %u", msg.cmd, msg.len, msg.data[0]);
            if (msg.cmd < BLE_CMD_MAX && ble_handlers[msg.cmd]) {
                ble_handlers[msg.cmd](msg.data, msg.len);
            }
        }
    }
}

int init_ble_task() {
    memset(ble_handlers, 0x00, sizeof(ble_handlers));
    return 0;
}

int start_ble_task() {
    return 0;
}