#include <zephyr/kernel.h>
#include "netwrk_task.h"
#include "loggers.h"

typedef struct {
    unsigned char cmd;
    unsigned char len;
    unsigned char data[28];
} netwrk_msg_t;

K_MSGQ_DEFINE(netwrk_msgq, sizeof(netwrk_msg_t), 16, 4);

ntwrk_msg_listner_t *netwrk_listeners[0x10];

int set_netwrk_listner(ntwrk_msg_listner_t *listner) {
    netwrk_listeners[listner->lid] = listner;
    return 0;
}

int push_netwrk_task(unsigned char cmd, unsigned char *data, unsigned char len) {
    if (len > 28) {
        return -EINVAL;
    }
    netwrk_msg_t msg = {0};
    msg.cmd = cmd;
    msg.len = len;
    memcpy(msg.data, data, len);
    return k_msgq_put(&netwrk_msgq, &msg, K_NO_WAIT);
}

void netwrk_th_cb(void *p1, void *p2, void *p3) {
    log_i("started");
    netwrk_msg_t msg;
    while (1) {
        if (!k_msgq_get(&netwrk_msgq, &msg, K_FOREVER)) {
            for (int i = 0; i < LID_MAX; i++) {
                ntwrk_msg_listner_t *listner = netwrk_listeners[i];
                if (listner) {
                    log_i("processing: %d, %d", msg.cmd, listner->count);
                    if (msg.cmd < listner->count) {
                        listner->cbs[msg.cmd](msg.data, msg.len);
                    }
                }
            }
           
        }
    }
}

K_THREAD_DEFINE(netwrk_th, 4096, netwrk_th_cb, NULL, NULL, NULL, 8, 0, 0);
int start_netwrk_task(void) {
    return 0;
}

int init_netwrk_task(void) {
    return 0;
}

