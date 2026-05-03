#include "clnt_a.h"
#include "data_commu.h"
#include "commu_msg.h"
#include "ca_cpa_msg.h"
#include "loggers.h"


static mmsg_handler_t clnt_a_handler = {
    .id = CLIENT_A,
    .cb ={
        [MSG_CPA] = decode_capa_msg,
    }
};

int init_clnt_a() {
    init_ca_cpa_msg();
    return 0;
}

int start_clnt_a() {
    start_ca_cpa_msg();
    set_client_handler(&clnt_a_handler);
    return 0;
}