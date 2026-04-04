#pragma once
#include "types.h"

#define MAX_DATA_COUNT    (600)
typedef struct __attribute__((__packed__)) {
    u8 flag;
    u8 spo2;
    u32 timestamp;
    u16 data_count;
    u8 data[MAX_DATA_COUNT];
} spo2_t;