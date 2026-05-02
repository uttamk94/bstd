#include "capa_msg.h"
#include "ca_cpa_msg.h"
#include "loggers.h"
#include "data_commu.h"

int add_feature_capa(unsigned char *buf) {
    int indx = 0;
    buf[indx++] = 0x01;
    buf[indx++] = 0x00;
    buf[indx++] = 0x00;
    buf[indx++] = 0x00;
    return indx;
}

void add_capa_header(msg_header_t *header) {
    header->msg_id  = 0x00;
    header->type    = 0;
    header->sz      = 0;
    header->resv    = 0;
}

void add_capa_feature(feature_t *feat) {
    feat->id = 0x01;
    feat->val = 0x00;
}

void add_capa_protocol(prtcl_t *prtcl) {
    prtcl->id = 0x01;
    prtcl->ver.major = 0x00;
    prtcl->ver.minor = 0x00;
    prtcl->ver.patch = 0x00;
}

int encode_capa_msg_req() {
    dev_capa_msg_t msg;
    add_capa_header(&msg.header);
    add_capa_feature(&msg.feat);
    add_capa_feature(&msg.sens);
    add_capa_feature(&msg.ver);
    add_capa_protocol(&msg.prtcl);
    return send_commu_data(&msg, sizeof(msg));
}

int encode_capa_msg_resp(seq_num_t *seq) {
    mob_capa_msg_t msg;
    add_capa_header(&msg.header);
    add_capa_feature(&msg.feat);
    add_capa_feature(&msg.sens);
    add_capa_feature(&msg.ver);
    add_capa_protocol(&msg.prtcl);
    return send_commu_data(&msg, sizeof(msg));
}

int decode_mobile_capa_msg(void *buff, unsigned short len) {
    mob_capa_msg_t *msg = (mob_capa_msg_t *) buff;
    msg->feat.id;
    msg->feat.val;
    return 0;
}

int decode_capa_msg_req(void *buff, unsigned short len) {
    return decode_mobile_capa_msg(buff, len);
}

int decode_capa_msg_resp(void *buff, unsigned short len) {
    return decode_mobile_capa_msg(buff, len);
}

int decode_capa_msg(void *buff, unsigned short len) {
    if (!buff || len < sizeof(mob_capa_msg_t)) {
        return -1;
    }
    
    msg_header_t *header = (msg_header_t *) buff;
    log_i("msg_id: %u, type: %u, sz: %u", header->msg_id, header->type, header->sz);
    if (header->type == 0) {
        encode_capa_msg_resp((seq_num_t *) buff + 1);
        decode_capa_msg_req(buff, len);
    } else {
        decode_capa_msg_resp(((unsigned char *)buff) + 1, len-1);
    }

    return 0;
}

int init_ca_cpa_msg() {
    return 0;
}

int start_ca_cpa_msg() {
    return 0;
}