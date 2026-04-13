#include "feature.h"
#include <stdio.h>

#include "ft_task.h"
#include "shandler.h"
#include "ft_a.h"
#include "ft_b.h"
#include "loggers.h"

int init_feature() {
    log_i("init_feature");
    init_ft_task();
    init_shandler();
    init_ft_a();
    init_ft_b();
    return 0;
}

int start_feature() {
    log_i("start_feature");
    start_ft_task();
    start_shandler();
    start_ft_a();
    start_ft_b();
    return 0;
}