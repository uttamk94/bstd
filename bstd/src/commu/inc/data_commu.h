#pragma once

typedef enum {
    CLIENT_A,
    CLIENT_B,
    CLIENT_C,
    CLIENT_D,
    CLIENT_MAX
} client_id_t;

typedef void (*msg_handler)(void *buf, unsigned short len);

typedef struct {
    client_id_t id;
   msg_handler handler;
} client_handler_t;

void set_client_handler(client_id_t client, msg_handler handler);
int send_commu_data(void *data, unsigned short len);

int init_data_commu();
int start_data_commu();