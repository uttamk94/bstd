#include <zephyr/kernel.h>
#include "clnt_b.h"
#include "capa_msg.h"
#include "data_commu.h"


static mmsg_handler_t clnt_b_handler = {
    .id = CLIENT_B,
    .cb ={
        [MSG_CPA] = NULL,
    }
};

int init_clnt_b() {
    return 0;
}

int start_clnt_b() {
    set_client_handler(&clnt_b_handler);
    return 0;
}