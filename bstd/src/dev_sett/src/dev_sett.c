#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>

#include "dev_sett.h"

#define MAX_LISTNER 0x10

typedef struct {
    ds_cmd_t cmd;
    unsigned int len;
    void *data;
} ds_msg_t;

K_MSGQ_DEFINE(ds_msgq, sizeof(ds_msg_t), 16, 4);

ds_listner_t *listeners[MAX_LISTNER];
ds_data_t ds_data = {0, };

ds_data_t get_ds_data() {
    return ds_data;
}

void set_ds_data(ds_data_t data) {
    ds_data = data;
}

int insert_dev_sett(ds_cmd_t cmd, unsigned int len, void *data) {
    ds_msg_t msg_copy = {cmd, len, data};
    return k_msgq_put(&ds_msgq, &msg_copy, K_NO_WAIT);
}

int set_ds_listner(ds_listner_t *listner) {
    for (int i = 0; i < MAX_LISTNER; i++) {
        if (!listeners[i]) {
            listeners[i] = listner;
            return 0;
        }
    }
    return -1;
}

void ds_chg_handler(unsigned int len, void *data) {
    printf("%s: %d\n", __func__, len);
    unsigned char *chg = (unsigned char *) data;
    for (int i = 0; i < MAX_LISTNER; i++) {
        if (listeners[i] && listeners[i]->chg_cb) {
            listeners[i]->chg_cb(*chg);
        }
    }
    free(chg);
    chg = NULL;
}

void ds_task_cb(void *p1, void *p2, void *p3) {
    printf("%s: started\n", __func__);
    ds_msg_t msg;
    while (1) {
        if (!k_msgq_get(&ds_msgq, &msg, K_FOREVER)) {
            switch (msg.cmd)
            {
            case DS_CHG_EVNT:
                ds_chg_handler(msg.len, msg.data);
                break;
            
            default:
                break;
            }
        }
    }
}

K_THREAD_DEFINE(ds_task, 1024, ds_task_cb, NULL, NULL, NULL, 12, 0, 0);

int init_dev_sett() {
    printf("init_dev_sett\n");
    return 0;
}

int start_dev_sett() {
    printf("start_dev_sett\n");
    return 0;
}