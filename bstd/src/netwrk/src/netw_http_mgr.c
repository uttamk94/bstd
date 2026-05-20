#include "netw_http_mgr.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

#define HTTP_REQ_SZ        256
#define HTTP_TIMEOUT_SEC   5

static int parse_http_rsp(
    char *buf,
    size_t len,
    netw_http_rsp_t *rsp)
{
    char *http;
    char *body;

    ARG_UNUSED(len);

    http = strstr(buf, "HTTP/");

    if (!http) {
        return -EINVAL;
    }

    rsp->status = atoi(http + 9);

    body = strstr(http, "\r\n\r\n");

    if (!body) {
        return -EINVAL;
    }

    body += 4;

    rsp->body = body;

    rsp->body_len = strlen(body);

    return 0;
}

static int send_all(
    int sock,
    const char *buf,
    size_t len)
{
    int ret;

    size_t sent = 0;

    while (sent < len) {

        ret = zsock_send(
            sock,
            buf + sent,
            len - sent,
            0);

        if (ret > 0) {

            sent += ret;

            continue;
        }

        if (errno == EAGAIN ||
            errno == ETIMEDOUT) {

            k_sleep(K_MSEC(20));

            continue;
        }

        return -errno;
    }

    return 0;
}

static int recv_all(
    int sock,
    char *buf,
    size_t buf_len)
{
    int ret;

    int len = 0;

    while (len < (buf_len - 1)) {

        ret = zsock_recv(
            sock,
            buf + len,
            buf_len - len - 1,
            0);

        if (ret > 0) {

            len += ret;

            continue;
        }

        if (ret == 0) {
            break;
        }

        if (errno == EAGAIN ||
            errno == ETIMEDOUT) {

            k_sleep(K_MSEC(20));

            continue;
        }

        return -errno;
    }

    buf[len] = 0;

    return len;
}

int netw_http_get(
    const char *ip,
    uint16_t port,
    const char *host,
    const char *path,
    char *buf,
    size_t buf_len,
    netw_http_rsp_t *rsp)
{
    int ret;

    int sock;

    char req[HTTP_REQ_SZ];

    struct timeval timeout = {
        .tv_sec = HTTP_TIMEOUT_SEC,
        .tv_usec = 0,
    };

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };

    if (!ip ||
        !host ||
        !path ||
        !buf ||
        !rsp) {

        return -EINVAL;
    }

    if (zsock_inet_pton(
            AF_INET,
            ip,
            &addr.sin_addr) != 1) {

        return -EINVAL;
    }

    sock = zsock_socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (sock < 0) {
        return -errno;
    }

    zsock_setsockopt(
        sock,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout));

    zsock_setsockopt(
        sock,
        SOL_SOCKET,
        SO_SNDTIMEO,
        &timeout,
        sizeof(timeout));

    ret = zsock_connect(
        sock,
        (struct sockaddr *)&addr,
        sizeof(addr));

    if (ret < 0) {

        ret = -errno;

        goto exit;
    }

    snprintk(
        req,
        sizeof(req),

        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n",

        path,
        host);

    ret = send_all(
        sock,
        req,
        strlen(req));

    if (ret < 0) {
        goto exit;
    }

    ret = recv_all(
        sock,
        buf,
        buf_len);

    if (ret < 0) {
        goto exit;
    }

    ret = parse_http_rsp(
        buf,
        ret,
        rsp);

exit:

    zsock_close(sock);

    return ret;
}