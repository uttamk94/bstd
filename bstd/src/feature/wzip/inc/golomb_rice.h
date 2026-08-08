/**
 * @file golomb_rice.h
 * @brief Golomb-Rice entropy coding for wzip compression
 *
 * Golomb-Rice coding is optimal for Laplacian-distributed residuals
 * (which is exactly what we get from pattern prediction errors).
 *
 * For well-matched patterns, residuals are tiny (±1-3), so k=1 or k=2
 * is optimal, giving ~2-3 bits per residual value.
 */

#ifndef WZIP_GOLOMB_RICE_H
#define WZIP_GOLOMB_RICE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Golomb-Rice encoder state
 */
typedef struct {
    uint8_t default_k;   /**< Default Golomb-Rice parameter */
    uint8_t max_k;       /**< Maximum k value */
} wzip_golomb_encoder_t;

/**
 * @brief Golomb-Rice bitstream writer
 */
typedef struct {
    uint8_t *buffer;     /**< Output byte buffer */
    uint16_t buffer_len; /**< Buffer length in bytes */
    uint16_t bit_pos;    /**< Current bit position */
} wzip_bitstream_writer_t;

/**
 * @brief Golomb-Rice bitstream reader
 */
typedef struct {
    const uint8_t *buffer; /**< Input byte buffer */
    uint16_t buffer_len;   /**< Buffer length in bytes */
    uint16_t bit_pos;      /**< Current bit position */
} wzip_bitstream_reader_t;

/**
 * @brief Initialize Golomb-Rice encoder
 *
 * @param enc      Encoder state
 * @param default_k Default k parameter (0-8)
 */
void wzip_golomb_init(wzip_golomb_encoder_t *enc, uint8_t default_k);

/**
 * @brief Find optimal k parameter for a set of residuals
 *
 * @param enc       Encoder state
 * @param residuals Residual values
 * @param n         Number of residuals
 * @return Optimal k value
 */
uint8_t wzip_golomb_optimal_k(wzip_golomb_encoder_t *enc,
                               const int32_t *residuals,
                               uint16_t n);

/**
 * @brief Initialize bitstream writer
 *
 * @param writer Bitstream writer
 * @param buffer Output buffer
 * @param len    Buffer length
 */
void wzip_writer_init(wzip_bitstream_writer_t *writer,
                      uint8_t *buffer, uint16_t len);

/**
 * @brief Write a single bit to the bitstream
 *
 * @param writer Bitstream writer
 * @param bit    Bit value (0 or 1)
 * @return 0 on success, -1 on buffer full
 */
int wzip_writer_write_bit(wzip_bitstream_writer_t *writer, uint8_t bit);

/**
 * @brief Write multiple bits to the bitstream
 *
 * @param writer Bitstream writer
 * @param value  Value to write
 * @param n_bits Number of bits to write
 * @return 0 on success, -1 on buffer full
 */
int wzip_writer_write_bits(wzip_bitstream_writer_t *writer,
                           uint32_t value, uint8_t n_bits);

/**
 * @brief Flush bitstream (pad to byte boundary)
 *
 * @param writer Bitstream writer
 * @return Number of padding bits added
 */
uint8_t wzip_writer_flush(wzip_bitstream_writer_t *writer);

/**
 * @brief Get total bytes written
 *
 * @param writer Bitstream writer
 * @return Number of bytes used
 */
uint16_t wzip_writer_bytes_written(wzip_bitstream_writer_t *writer);

/**
 * @brief Initialize bitstream reader
 *
 * @param reader Bitstream reader
 * @param buffer Input buffer
 * @param len    Buffer length
 */
void wzip_reader_init(wzip_bitstream_reader_t *reader,
                      const uint8_t *buffer, uint16_t len);

/**
 * @brief Read a single bit from the bitstream
 *
 * @param reader Bitstream reader
 * @return Bit value (0 or 1), or -1 on end of stream
 */
int wzip_reader_read_bit(wzip_bitstream_reader_t *reader);

/**
 * @brief Read multiple bits from the bitstream
 *
 * @param reader Bitstream reader
 * @param n_bits Number of bits to read
 * @return Value read, or -1 on end of stream
 */
int32_t wzip_reader_read_bits(wzip_bitstream_reader_t *reader, uint8_t n_bits);

/**
 * @brief Encode a single value with Golomb-Rice
 *
 * @param writer Bitstream writer
 * @param value  Value to encode (can be negative)
 * @param k      Golomb-Rice parameter
 * @return 0 on success, error code otherwise
 */
int wzip_golomb_encode_value(wzip_bitstream_writer_t *writer,
                              int32_t value, uint8_t k);

/**
 * @brief Decode a single Golomb-Rice value
 *
 * @param reader Bitstream reader
 * @param k      Golomb-Rice parameter
 * @return Decoded value
 */
int32_t wzip_golomb_decode_value(wzip_bitstream_reader_t *reader, uint8_t k);

/**
 * @brief Encode a block of residuals with optimal k
 *
 * @param writer    Bitstream writer
 * @param residuals Residual array
 * @param n         Number of residuals
 * @param enc       Encoder state
 * @return 0 on success, error code otherwise
 */
int wzip_golomb_encode_block(wzip_bitstream_writer_t *writer,
                              const int32_t *residuals,
                              uint16_t n,
                              wzip_golomb_encoder_t *enc);

/**
 * @brief Decode a block of residuals
 *
 * @param reader Bitstream reader
 * @param n      Number of residuals to decode
 * @param output Output array for decoded values
 * @return 0 on success, error code otherwise
 */
int wzip_golomb_decode_block(wzip_bitstream_reader_t *reader,
                              uint16_t n, int32_t *output);

#ifdef __cplusplus
}
#endif

#endif /* WZIP_GOLOMB_RICE_H */