#pragma once
#include "message.h"

typedef enum {
    CLIENT_A,
    CLIENT_B,
    CLIENT_C,
    CLIENT_D,
    CLIENT_MAX
} client_id_t;

typedef void (*msg_cb)(void *buf, unsigned short len);

typedef struct {
    client_id_t id;
    msg_cb cb[MSG_MAX];
} mmsg_handler_t;

void set_client_handler(mmsg_handler_t *handler);
int send_commu_data(void *data, unsigned short len);

int init_data_commu();
int start_data_commu();