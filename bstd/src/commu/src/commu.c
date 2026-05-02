
#include "commu.h"


int init_commu() {
    init_data_commu();
    init_clnt_a();
    return 0;
}


int start_commu() {
    start_data_commu();
    start_clnt_a();
    return 0;
}