/**
 * @file golomb_rice.c
 * @brief Golomb-Rice entropy coding implementation
 *
 * Optimized for Cortex-M4+ with fixed-point arithmetic.
 * No floating-point operations, no division (uses shifts).
 */

#include "golomb_rice.h"
#include <string.h>

void wzip_golomb_init(wzip_golomb_encoder_t *enc, uint8_t default_k)
{
    enc->default_k = default_k;
    enc->max_k = 8;
}

uint8_t wzip_golomb_optimal_k(wzip_golomb_encoder_t *enc,
                               const int32_t *residuals,
                               uint16_t n)
{
    uint8_t best_k = enc->default_k;
    uint32_t best_bits = 0xFFFFFFFF;
    
    for (uint8_t k = 0; k <= enc->max_k; k++) {
        uint32_t total_bits = 0;
        
        for (uint16_t i = 0; i < n; i++) {
            uint32_t abs_val = (residuals[i] < 0) ? 
                               (uint32_t)(-residuals[i]) : 
                               (uint32_t)(residuals[i]);
            
            /* sign(1) + unary(quotient+1) + remainder(k) */
            uint32_t quotient = abs_val >> k;
            total_bits += 1 + quotient + 1 + k;
        }
        
        if (total_bits < best_bits) {
            best_bits = total_bits;
            best_k = k;
        }
    }
    
    return best_k;
}

/* Bitstream writer implementation */
void wzip_writer_init(wzip_bitstream_writer_t *writer,
                      uint8_t *buffer, uint16_t len)
{
    writer->buffer = buffer;
    writer->buffer_len = len;
    writer->bit_pos = 0;
    memset(buffer, 0, len);
}

int wzip_writer_write_bit(wzip_bitstream_writer_t *writer, uint8_t bit)
{
    uint16_t byte_pos = writer->bit_pos >> 3;
    uint8_t  bit_offset = writer->bit_pos & 7;
    
    if (byte_pos >= writer->buffer_len) {
        return -1; /* Buffer full */
    }
    
    if (bit) {
        writer->buffer[byte_pos] |= (1 << (7 - bit_offset));
    }
    
    writer->bit_pos++;
    return 0;
}

int wzip_writer_write_bits(wzip_bitstream_writer_t *writer,
                           uint32_t value, uint8_t n_bits)
{
    for (int8_t i = n_bits - 1; i >= 0; i--) {
        uint8_t bit = (value >> i) & 1;
        if (wzip_writer_write_bit(writer, bit) != 0) {
            return -1;
        }
    }
    return 0;
}

uint8_t wzip_writer_flush(wzip_bitstream_writer_t *writer)
{
    uint8_t padding = (8 - (writer->bit_pos & 7)) & 7;
    for (uint8_t i = 0; i < padding; i++) {
        wzip_writer_write_bit(writer, 0);
    }
    return padding;
}

uint16_t wzip_writer_bytes_written(wzip_bitstream_writer_t *writer)
{
    return (writer->bit_pos + 7) >> 3;
}

/* Bitstream reader implementation */
void wzip_reader_init(wzip_bitstream_reader_t *reader,
                      const uint8_t *buffer, uint16_t len)
{
    reader->buffer = buffer;
    reader->buffer_len = len;
    reader->bit_pos = 0;
}

int wzip_reader_read_bit(wzip_bitstream_reader_t *reader)
{
    uint16_t byte_pos = reader->bit_pos >> 3;
    uint8_t  bit_offset = reader->bit_pos & 7;
    
    if (byte_pos >= reader->buffer_len) {
        return -1; /* End of stream */
    }
    
    int bit = (reader->buffer[byte_pos] >> (7 - bit_offset)) & 1;
    reader->bit_pos++;
    return bit;
}

int32_t wzip_reader_read_bits(wzip_bitstream_reader_t *reader, uint8_t n_bits)
{
    int32_t value = 0;
    for (uint8_t i = 0; i < n_bits; i++) {
        int bit = wzip_reader_read_bit(reader);
        if (bit < 0) return -1;
        value = (value << 1) | bit;
    }
    return value;
}

/* Golomb-Rice encode/decode */
int wzip_golomb_encode_value(wzip_bitstream_writer_t *writer,
                              int32_t value, uint8_t k)
{
    /* Sign bit: 0 for positive/zero, 1 for negative */
    uint8_t sign = (value < 0) ? 1 : 0;
    uint32_t abs_val = (value < 0) ? (uint32_t)(-value) : (uint32_t)value;
    
    /* Quotient and remainder */
    uint32_t quotient = abs_val >> k;
    uint32_t remainder = abs_val & ((1 << k) - 1);
    
    /* Write sign bit */
    if (wzip_writer_write_bit(writer, sign) != 0) return -1;
    
    /* Write unary code: quotient ones followed by 0 */
    for (uint32_t i = 0; i < quotient; i++) {
        if (wzip_writer_write_bit(writer, 1) != 0) return -1;
    }
    if (wzip_writer_write_bit(writer, 0) != 0) return -1;
    
    /* Write remainder in k bits */
    if (k > 0) {
        if (wzip_writer_write_bits(writer, remainder, k) != 0) return -1;
    }
    
    return 0;
}

int32_t wzip_golomb_decode_value(wzip_bitstream_reader_t *reader, uint8_t k)
{
    /* Read sign bit */
    int sign = wzip_reader_read_bit(reader);
    if (sign < 0) return 0;
    
    /* Read unary code: count ones until we hit 0 */
    uint32_t quotient = 0;
    while (1) {
        int bit = wzip_reader_read_bit(reader);
        if (bit < 0) return 0;
        if (bit == 0) break;
        quotient++;
    }
    
    /* Read remainder in k bits */
    uint32_t remainder = 0;
    if (k > 0) {
        int32_t r = wzip_reader_read_bits(reader, k);
        if (r < 0) return 0;
        remainder = (uint32_t)r;
    }
    
    /* Reconstruct value */
    int32_t abs_val = (int32_t)((quotient << k) + remainder);
    return sign ? -abs_val : abs_val;
}

int wzip_golomb_encode_block(wzip_bitstream_writer_t *writer,
                              const int32_t *residuals,
                              uint16_t n,
                              wzip_golomb_encoder_t *enc)
{
    /* Find optimal k */
    uint8_t k = wzip_golomb_optimal_k(enc, residuals, n);
    
    /* Write k as 4-bit header */
    if (wzip_writer_write_bits(writer, k, 4) != 0) return -1;
    
    /* Encode each residual */
    for (uint16_t i = 0; i < n; i++) {
        if (wzip_golomb_encode_value(writer, residuals[i], k) != 0) {
            return -1;
        }
    }
    
    return 0;
}

int wzip_golomb_decode_block(wzip_bitstream_reader_t *reader,
                              uint16_t n, int32_t *output)
{
    /* Read k from 4-bit header */
    int32_t k = wzip_reader_read_bits(reader, 4);
    if (k < 0) return -1;
    
    /* Decode each residual */
    for (uint16_t i = 0; i < n; i++) {
        output[i] = wzip_golomb_decode_value(reader, (uint8_t)k);
    }
    
    return 0;
}