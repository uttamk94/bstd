#include <zephyr/kernel.h>
#include <stdio.h>
#include "shandler.h"
#include "sensor.h"
#include "ft_task.h"
#include "loggers.h"
#define MAX_HANDLER 0X40

sns_handler_t *sns_handlers[MAX_HANDLER];


void on_sensor_data_received(sens_type_t type, unsigned int len, void *data) {
    log_i("%d, %u", type, len);
    insert_msg_data(CMD_SENSOR, type, len, data);
}

void on_msg_handler(msg_t *msg) {
    log_i("%d, %u", msg->type, msg->len);
    for (int i = 0; i < MAX_HANDLER; i++) {
        if (sns_handlers[i] && sns_handlers[i]->type == msg->type) {
            sns_handlers[i]->sensor_data_cb(msg->type, msg->len, msg->data);
        }
    }
}

int add_sensor(sns_handler_t *handler) {
    bool found = false;
    for (int i = 0; i < MAX_HANDLER; i++) {
        if (sns_handlers[i] && sns_handlers[i]->type == handler->type) {
            found = true;
            break;
        }
    }
    sns_handlers[handler->cid] = handler;
    if (!found) {
        reg_sensor(handler->type, on_sensor_data_received);
    }
    log_i("f:%d t:%u c: %u",  found, handler->type,handler->cid);
    return found;
}

int del_sensor(sns_handler_t *handler) {
    bool found = false;
    sns_handlers[handler->cid] = NULL;
    for (int i = 0; i < MAX_HANDLER; i++) {
        if (sns_handlers[i] && sns_handlers[i]->type == handler->type) {
            found = true;
            break;
        }
    }
    if (!found) {
        unreg_sensor(handler->type, on_sensor_data_received);
    }
    log_i("f:%d t:%u c: %u", found, handler->type,handler->cid);
    return found;

}

int add_all_sensor() {
    return 0;
}

int del_all_sensor() {
    return 0;
}

int init_shandler() {
    log_i("init_shandler");
    memset(sns_handlers, 0, sizeof(sns_handler_t *) * MAX_HANDLER);
    return 0;
}

int start_shandler() {
    log_i("start_shandler");
    reg_msg_handler(CMD_SENSOR, on_msg_handler);
    return 0;
}