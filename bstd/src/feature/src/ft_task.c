#include <zephyr/kernel.h>
#include "ft_task.h"
#include "loggers.h"

#define MAX_CLNT 0X10
K_MSGQ_DEFINE(msg_q, sizeof(msg_t), 16, 4);
K_MUTEX_DEFINE(msg_mutex);

msg_handler handlers[MAX_CMD][MAX_CLNT];

void reg_msg_handler(unsigned char cmd, msg_handler handler) {
    log_i("%d", cmd);
    if (cmd >= MAX_CMD || !handler) return;
    for (int i = 0; i < MAX_CLNT; i++) {
        if (!handlers[cmd][i]) {
            handlers[cmd][i] = handler;
            return;
        }
    }
    return;
}

int insert_msg(msg_t *msg) {
    msg_t msg_copy = *msg;
    return k_msgq_put(&msg_q, &msg_copy, K_NO_WAIT);
}

int insert_msg_data(cmd_t cmd, unsigned char type, unsigned int len, void *data){
    if (!data || len == 0 || len > 0x1000) {
        log_e("Inv cmd=%d, len=%d", cmd, len);
        return -EINVAL;
    }

    msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd = cmd;
    msg.type = type;
    msg.len = len;
    memcpy(msg.data, data, len);
    if (k_mutex_lock(&msg_mutex, K_FOREVER) != 0) {
        log_e("Failed to acquire mutex");
        return -EACCES;
    }
    if (k_msgq_put(&msg_q, &msg, K_NO_WAIT)){
        log_e("Queue full, dropping cmd %d", cmd);
    }
    k_mutex_unlock(&msg_mutex);
    return 0;
}

void ft_task_cb(void *p1, void *p2, void *p3) {
    log_i("started");
    msg_t msg;
    while (1) {
        if (!k_msgq_get(&msg_q, &msg, K_FOREVER)) {
            for (int i = 0; i < MAX_CLNT; i++) {
                if (handlers[msg.cmd][i]) {
                    log_i("%d", i);
                    handlers[msg.cmd][i](&msg);
                }
            }
        }
    }
}


int init_ft_task() {
    memset(handlers, 0, sizeof(handlers));
    return 0;
}

int start_ft_task(void) {
    log_i("Starting feature task");
    return 0;
}

int stop_ft_task(void) {
    log_i("Stopping feature task");
    /* Note: Thread cannot be easily stopped in Zephyr without k_thread_abort */
    return 0;
}

K_THREAD_DEFINE(ft_task, 1024 * 12, ft_task_cb, NULL, NULL, NULL, 12, 0, 0);
