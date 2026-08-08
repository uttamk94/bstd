/**
 * @file tlv_container.c
 * @brief TLV container implementation
 */

#include "tlv_container.h"
#include <string.h>

uint16_t wzip_container_init(uint8_t *buffer, uint16_t capacity)
{
    if (capacity < 6) return 0;
    
    wzip_container_header_t *hdr = (wzip_container_header_t *)buffer;
    hdr->magic[0] = 'W';
    hdr->magic[1] = 'Z';
    hdr->magic[2] = 'I';
    hdr->magic[3] = 'P';
    hdr->num_chunks = 0;
    
    return sizeof(wzip_container_header_t);
}

int wzip_container_add_chunk(uint8_t *buffer, uint16_t *pos,
                              uint16_t capacity,
                              uint8_t type,
                              const uint8_t *value,
                              uint16_t val_len)
{
    uint16_t total_len = sizeof(wzip_tlv_header_t) + val_len;
    
    if (*pos + total_len > capacity) {
        return -1; /* Buffer full */
    }
    
    wzip_tlv_header_t *hdr = (wzip_tlv_header_t *)(buffer + *pos);
    hdr->type = type;
    hdr->length = val_len;
    
    if (val_len > 0 && value != NULL) {
        memcpy(buffer + *pos + sizeof(wzip_tlv_header_t), value, val_len);
    }
    
    *pos += total_len;
    
    /* Update chunk count in container header */
    wzip_container_header_t *container = (wzip_container_header_t *)buffer;
    container->num_chunks++;
    
    return 0;
}

int wzip_container_add_sensor(uint8_t *buffer, uint16_t *pos,
                               uint16_t capacity,
                               uint8_t channel,
                               uint8_t pattern_idx,
                               int16_t scale_factor,
                               const uint8_t *residual,
                               uint16_t residual_bits,
                               uint8_t padding)
{
    /* Map channel to type */
    uint8_t types[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    };
    
    if (channel >= 8) return -1;
    uint8_t type = types[channel];
    
    /* Build payload: header + residual bytes */
    wzip_compressed_header_t payload;
    payload.pattern_idx = pattern_idx;
    payload.scale_factor = scale_factor;
    payload.residual_padding = padding;
    payload.residual_bit_len = residual_bits;
    
    /* Calculate residual byte length */
    uint16_t residual_bytes = (residual_bits + 7) >> 3;
    
    uint16_t total_payload = sizeof(wzip_compressed_header_t) + residual_bytes;
    
    /* Add as TLV chunk */
    uint16_t chunk_pos = *pos + sizeof(wzip_tlv_header_t);
    
    if (chunk_pos + total_payload > capacity) {
        return -1;
    }
    
    /* Write header */
    wzip_tlv_header_t *hdr = (wzip_tlv_header_t *)(buffer + *pos);
    hdr->type = type;
    hdr->length = total_payload;
    
    /* Write payload header */
    memcpy(buffer + chunk_pos, &payload, sizeof(wzip_compressed_header_t));
    
    /* Write residual data */
    if (residual_bytes > 0 && residual != NULL) {
        memcpy(buffer + chunk_pos + sizeof(wzip_compressed_header_t), 
               residual, residual_bytes);
    }
    
    *pos += sizeof(wzip_tlv_header_t) + total_payload;
    
    /* Update chunk count */
    wzip_container_header_t *container = (wzip_container_header_t *)buffer;
    container->num_chunks++;
    
    return 0;
}

int wzip_container_finalize(uint8_t *buffer, uint16_t *pos, uint16_t capacity)
{
    /* Add end-of-stream marker (type=0xFF, length=0) */
    return wzip_container_add_chunk(buffer, pos, capacity, 0xFF, NULL, 0);
}

int wzip_container_parse_header(const uint8_t *data, uint16_t len,
                                 uint16_t *num_chunks)
{
    if (len < sizeof(wzip_container_header_t)) {
        return -1;
    }
    
    const wzip_container_header_t *hdr = 
        (const wzip_container_header_t *)data;
    
    /* Check magic */
    if (hdr->magic[0] != 'W' || hdr->magic[1] != 'Z' ||
        hdr->magic[2] != 'I' || hdr->magic[3] != 'P') {
        return -2;
    }
    
    *num_chunks = hdr->num_chunks;
    
    return sizeof(wzip_container_header_t);
}

int wzip_container_next_chunk(const uint8_t *data, uint16_t len,
                               uint16_t *offset,
                               uint8_t *type,
                               const uint8_t **value,
                               uint16_t *val_len)
{
    if (*offset + sizeof(wzip_tlv_header_t) > len) {
        return -1; /* End of stream */
    }
    
    const wzip_tlv_header_t *hdr = 
        (const wzip_tlv_header_t *)(data + *offset);
    
    *type = hdr->type;
    *val_len = hdr->length;
    
    uint16_t value_offset = *offset + sizeof(wzip_tlv_header_t);
    
    if (value_offset + hdr->length > len) {
        return -2; /* Corrupted data */
    }
    
    *value = data + value_offset;
    *offset = value_offset + hdr->length;
    
    return 0;
}