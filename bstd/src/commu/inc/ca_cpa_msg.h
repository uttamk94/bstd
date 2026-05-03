#pragma once
#include "capa_msg.h"

void decode_capa_msg(void *buf, unsigned short len);
int init_ca_cpa_msg();
int start_ca_cpa_msg();