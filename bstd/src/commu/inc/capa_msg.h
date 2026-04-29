#pragma once

typedef struct {
    unsigned char msg_id:5;     /* message id 0 ~ 31 */
    unsigned char type:1;       /* 0: request, 1: response */
    unsigned char sz:1;         /* 0: variable, 1: fixed */
    unsigned char resv:1;       /* reserved */
} msg_header_t;

typedef struct {
    unsigned char key;
    unsigned short value;
} seq_num_t;

typedef struct {
    unsigned char id;
    unsigned char val;
} feature_t;

typedef struct {
    unsigned char id;
    struct {
        unsigned char major;
        unsigned char minor;
        unsigned char patch;
        unsigned char resv;
    } ver;
} prtcl_t;

typedef struct __attribute__((packed)) {
    msg_header_t header;
    seq_num_t   seq;
    feature_t   feat;
    feature_t   sens;
    feature_t   ver;
    prtcl_t     prtcl;
} dev_capa_msg_t;

typedef struct __attribute__((packed)) {
    msg_header_t header;
    feature_t   feat;
    feature_t   sens;
    feature_t   ver;
    prtcl_t     prtcl;
} mob_capa_msg_t;