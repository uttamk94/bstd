#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int status;
    char *body;
    size_t body_len;
} netw_http_rsp_t;

int netw_http_get(const char *ip,
                  uint16_t port,
                  const char *host,
                  const char *path,
                  char *buf,
                  size_t buf_len,
                  netw_http_rsp_t *rsp);