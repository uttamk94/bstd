#pragma once

typedef struct {
    unsigned char cmd;
    unsigned int len;
    unsigned char *data;
} msg_t;

typedef void (*msg_handler)(msg_t *msg);

void reg_msg_handler(unsigned char cmd, msg_handler handler);
int insert_msg(msg_t *msg);
int insert_msg_data(unsigned char cmd, unsigned int len, unsigned char *data);

int init_ft_task();
