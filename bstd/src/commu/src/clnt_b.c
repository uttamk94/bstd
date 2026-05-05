#include <zephyr/kernel.h>
#include "clnt_b.h"
#include "capa_msg.h"
#include "data_commu.h"
#include "cb_cpa_msg.h"


static void decode_req_msg(void *buf, unsigned short len) {
    //req_msg_t *msg = (req_msg_t *) buf;
}

static void decode_sync_msg(void *buf, unsigned short len) {
    //mob_capa_msg_t *msg = (mob_capa_msg_t *) buf;
}


static mmsg_handler_t clnt_b_handler = {
    .id = CLIENT_B,
    .count = 0,
    .cbs ={
        { .id = MSG_CPA, .msg_cb = NULL },
        { .id = MSG_REQ, .msg_cb = decode_req_msg },
        { .id = MSG_SYNC, .msg_cb = decode_sync_msg },
    }
};

int init_clnt_b() {
    //init_cb_cpa_msg();
    return 0;
}

int start_clnt_b() {
    //start_cb_cpa_msg();
    set_client_handler(&clnt_b_handler);
    return 0;
}