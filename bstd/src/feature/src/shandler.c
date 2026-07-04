#include <zephyr/kernel.h>
#include <stdio.h>
#include "shandler.h"
#include "sensor.h"
#include "ft_task.h"
#include "loggers.h"

/* Sensor handler module */
#define MAX_HANDLER 0X40

sns_handler_t *sns_handlers[MAX_HANDLER];



static void on_sensor_data_received(sens_type_t type, unsigned int len, void *data) {
    log_i("%d, %u", type, len);
    insert_msg_data(CMD_SENSOR, (unsigned char)type, len, data);
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
    if (!handler) {
        log_e("Inv");
        return false;
    }
    bool found = false;
    for (int i = 0; i < MAX_HANDLER; i++) {
        if (sns_handlers[i] && sns_handlers[i]->type == handler->type) {
            found = true;
            break;
        }
    }
    sns_handlers[handler->cid] = handler;
    if (!found) {
        int ret = reg_sensor(handler->type, on_sensor_data_received);
        if (ret != 0) {
            log_e("type %u", handler->type);
            return false;
        }
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

int init_shandler(void) {
    log_i("Init");
    memset(sns_handlers, 0, sizeof(sns_handler_t *) * MAX_HANDLER);
    return 0;
}

int start_shandler(void) {
    log_i("Starting sensor handler");
    reg_msg_handler(CMD_SENSOR, on_msg_handler);
    return 0;
}

int stop_shandler(void) {
    log_i("Stop");
    memset(sns_handlers, 0, sizeof(sns_handler_t *) * MAX_HANDLER);
    return 0;
}
