#pragma once

typedef enum {
    LID_MAIN,
    LID_MAX
} ntwrk_listner_id_t;

typedef enum {
    NTWRK_CMD_CONNECT,
    NTWRK_CMD_DISCONNECT,
    NTWRK_CMD_DATA,
    NTWRK_CMD_MAX
} ntwrk_cmd_t;

typedef void (*ntwrk_cb)(void *data, unsigned char len);
typedef struct {
    ntwrk_listner_id_t lid;
    unsigned char count;
    ntwrk_cb cbs[];
} ntwrk_msg_listner_t;

int set_netwrk_listner(ntwrk_msg_listner_t *listner);
int push_netwrk_task(unsigned char cmd, unsigned char *data, unsigned char len);
int start_netwrk_task(void);
int init_netwrk_task(void);

