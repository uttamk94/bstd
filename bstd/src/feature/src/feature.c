#include "feature.h"
#include <stdio.h>

#include "ft_task.h"
#include "ft_a.h"
#include "ft_b.h"






int init_feature() {
    printf("init_feature\n");
    init_ft_task();
    init_ft_a();
    init_ft_b();
    return 0;
}

int start_feature() {
    printf("start_feature\n");
    start_ft_a();
    start_ft_b();
    return 0;
}