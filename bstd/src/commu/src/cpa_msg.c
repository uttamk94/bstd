#include "capa_msg.h"


typedef struct {
    struct {
        unsigned char id;
        unsigned char val;
    } feat, sens, ver;
    struct {
        unsigned char id;
        unsigned int ver;
    } prtcl;
    struct {
        unsigned char id;
        unsigned char val;
    } sensor;
} capa_msg_t;

int add_feature_capa(unsigned char *buf) {
    int indx = 0;
    buf[indx++] = 0x01;
    buf[indx++] = 0x00;
    buf[indx++] = 0x00;
    buf[indx++] = 0x00;
    return indx;
}

int encode_capa_msg_req(unsigned char client) {
    unsigned char buf[64];
    int indx  = 0;
    indx += add_feature_capa(buf+indx);
    return send_msg(buf, indx);
}

int decode_capa_msg_res(void *buff, unsigned short len) {
    return 0;
}