#pragma once

typedef enum {
    MSG_ID_A = 0,
    MSG_ID_B,
    MSG_ID_C,
    MSG_ID_MAX
} msg_id_t;

typedef struct {
    msg_id_t msg_id;
    int (*msg_cb)(void *buf, unsigned short len);
} commu_msg_t;

