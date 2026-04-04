#pragma once

typedef enum {
    CMD_NONE = 0x00,
    CMD_SENSOR,
    CMD_CHG_EVENT,
    CMD_OOBE_EVENT,
    CMD_PSM_EVENT,
    MAX_CMD
} cmd_t;

typedef struct {
    unsigned char cmd;
    unsigned char type;
    unsigned int len;
    unsigned char *data;
} msg_t;

typedef void (*msg_handler)(msg_t *msg);

void reg_msg_handler(unsigned char cmd, msg_handler handler);
int insert_msg(msg_t *msg);
int insert_msg_data(unsigned char cmd, unsigned int len, unsigned char *data);

int init_ft_task();
int start_ft_task();
