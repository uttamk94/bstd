#include "clnt_a.h"
#include "data_commu.h"
#include "commu_msg.h"
#include "ca_cpa_msg.h"
#include "loggers.h"

#define ARY_SZ(ary) (sizeof(ary) / sizeof(ary[0]))

commu_msg_t msg_table[] = {
    {MSG_ID_A, decode_capa_msg},
    {MSG_ID_B, NULL},
    {MSG_ID_C, NULL}
};

static void on_data_received(void *buf, unsigned short len) {
    if (!buf || len == 0) {
       return; 
    }

    unsigned char msgid = ((unsigned char *)buf)[0];
    log_i("msg_id: %u", msgid);
    for (int i = 0; i < ARY_SZ(msg_table); i++) {
        if (msg_table[i].msg_id == msgid) {
            msg_table[i].msg_cb(buf, len);
        }
    }
}

int init_clnt_a() {
    init_ca_cpa_msg();
    return 0;
}

int start_clnt_a() {
    start_ca_cpa_msg();
    set_client_handler(CLIENT_A, on_data_received);
    return 0;
}