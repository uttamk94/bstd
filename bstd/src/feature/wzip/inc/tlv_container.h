/**
 * @file tlv_container.h
 * @brief TLV container format for wzip compressed streams
 *
 * Each chunk: [TYPE: 1 byte][LEN: 2 bytes][VALUE: LEN bytes]
 * Container: [MAGIC: 4 bytes][NUM_CHUNKS: 2 bytes][chunks...]
 */

#ifndef WZIP_TLV_CONTAINER_H
#define WZIP_TLV_CONTAINER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Magic number */
#define WZIP_MAGIC_BYTES { 'W', 'Z', 'I', 'P' }

/**
 * @brief TLV chunk header (3 bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t  type;    /**< Chunk type */
    uint16_t length;  /**< Payload length */
} wzip_tlv_header_t;

/**
 * @brief Container header (6 bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t  magic[4]; /**< "WZIP" */
    uint16_t num_chunks; /**< Number of chunks */
} wzip_container_header_t;

/**
 * @brief Compressed sensor payload header
 */
typedef struct __attribute__((packed)) {
    uint8_t  pattern_idx;      /**< Pattern index (0-255) */
    int16_t  scale_factor;     /**< Scale factor */
    uint8_t  residual_padding; /**< Padding bits in last byte */
    uint16_t residual_bit_len; /**< Total residual bits */
} wzip_compressed_header_t;

/**
 * @brief Initialize a TLV container in a buffer
 *
 * @param buffer Output buffer
 * @param capacity Buffer capacity
 * @return Position after header (first chunk starts here)
 */
uint16_t wzip_container_init(uint8_t *buffer, uint16_t capacity);

/**
 * @brief Add a TLV chunk to the container
 *
 * @param buffer    Output buffer
 * @param pos       Current position (updated on return)
 * @param capacity  Buffer capacity
 * @param type      Chunk type
 * @param value     Payload data
 * @param val_len   Payload length
 * @return 0 on success, -1 on buffer full
 */
int wzip_container_add_chunk(uint8_t *buffer, uint16_t *pos,
                              uint16_t capacity,
                              uint8_t type,
                              const uint8_t *value,
                              uint16_t val_len);

/**
 * @brief Add a compressed sensor chunk
 *
 * @param buffer        Output buffer
 * @param pos           Current position
 * @param capacity      Buffer capacity
 * @param channel       Channel index (0-7)
 * @param pattern_idx   Pattern index
 * @param scale_factor  Scale factor
 * @param residual      Residual bytes (Golomb-Rice encoded)
 * @param residual_bits Length of residual in bits
 * @param padding       Padding bits in last byte
 * @return 0 on success, -1 on buffer full
 */
int wzip_container_add_sensor(uint8_t *buffer, uint16_t *pos,
                               uint16_t capacity,
                               uint8_t channel,
                               uint8_t pattern_idx,
                               int16_t scale_factor,
                               const uint8_t *residual,
                               uint16_t residual_bits,
                               uint8_t padding);

/**
 * @brief Add end-of-stream marker
 *
 * @param buffer   Output buffer
 * @param pos      Current position
 * @param capacity Buffer capacity
 * @return 0 on success
 */
int wzip_container_finalize(uint8_t *buffer, uint16_t *pos, uint16_t capacity);

/**
 * @brief Parse a TLV container header
 *
 * @param data     Input data
 * @param len      Data length
 * @param num_chunks Output: number of chunks
 * @return Offset to first chunk, or -1 on error
 */
int wzip_container_parse_header(const uint8_t *data, uint16_t len,
                                 uint16_t *num_chunks);

/**
 * @brief Read next TLV chunk
 *
 * @param data      Input data
 * @param len       Data length
 * @param offset    Current offset (updated on return)
 * @param type      Output: chunk type
 * @param value     Output: pointer to payload within data
 * @param val_len   Output: payload length
 * @return 0 on success, -1 on end of stream, -2 on error
 */
int wzip_container_next_chunk(const uint8_t *data, uint16_t len,
                               uint16_t *offset,
                               uint8_t *type,
                               const uint8_t **value,
                               uint16_t *val_len);

#ifdef __cplusplus
}
#endif

#endif /* WZIP_TLV_CONTAINER_H */