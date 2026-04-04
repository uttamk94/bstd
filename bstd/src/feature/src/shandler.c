#include <zephyr/kernel.h>
#include <stdio.h>
#include "shandler.h"
#include "sensor.h"
#include "ft_task.h"

#define MAX_HANDLER 0X40

sns_handler_t *sns_handlers[MAX_HANDLER];



void on_sensor_data_received(sens_type_t type, unsigned int len, void *data) {
    printf("%s %d, %u,\n", __func__, type, len);
    insert_msg_data(type, len, data);
}

void on_msg_handler(msg_t *msg) {
    printf("%s %d, %u,\n", __func__, msg->type, msg->len);
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
    printf("%s: f:%d t:%u c: %u\n", __func__, found, handler->type,handler->cid);
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
    printf("%s: f:%d t:%u c: %u\n", __func__, found, handler->type,handler->cid);
    return found;

}

int add_all_sensor() {
    return 0;
}

int del_all_sensor() {
    return 0;
}

int init_shandler() {
    return 0;
}

int start_shandler() {
    reg_msg_handler(CMD_SENSOR, on_msg_handler);
    return 0;
}