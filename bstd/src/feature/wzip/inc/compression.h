/**
 * @file compression.h
 * @brief Main API for wzip wearable compression module
 *
 * Adaptive lossless compression for wearable sensor data using
 * learned pattern dictionaries. Designed for resource-constrained
 * devices (Cortex-M4+, ESP32-S3).
 *
 * Usage:
 *   1. Initialize with wzip_init()
 *   2. Feed sensor data with wzip_compress_window()
 *   3. Get compressed output with wzip_get_output()
 *   4. Decode with wzip_decompress()
 */

#ifndef WZIP_COMPRESSION_H
#define WZIP_COMPRESSION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default configuration (can be overridden via Kconfig) */
#ifndef CONFIG_COMPRESSION_CODEBOOK_SIZE
#define CONFIG_COMPRESSION_CODEBOOK_SIZE 256
#endif

#ifndef CONFIG_COMPRESSION_WINDOW_SIZE
#define CONFIG_COMPRESSION_WINDOW_SIZE 64
#endif

#ifndef CONFIG_COMPRESSION_NUM_CHANNELS
#define CONFIG_COMPRESSION_NUM_CHANNELS 8
#endif

#ifndef CONFIG_COMPRESSION_GOLOMB_RICE_K
#define CONFIG_COMPRESSION_GOLOMB_RICE_K 2
#endif

#ifndef CONFIG_COMPRESSION_NOVELTY_THRESHOLD
#define CONFIG_COMPRESSION_NOVELTY_THRESHOLD 1000
#endif

/* TLV chunk types */
#define WZIP_TYPE_PPG         0x01
#define WZIP_TYPE_ACCEL_X     0x02
#define WZIP_TYPE_ACCEL_Y     0x03
#define WZIP_TYPE_ACCEL_Z     0x04
#define WZIP_TYPE_TEMP        0x05
#define WZIP_TYPE_GYRO_X      0x06
#define WZIP_TYPE_GYRO_Y      0x07
#define WZIP_TYPE_GYRO_Z      0x08
#define WZIP_TYPE_RAW_SENSOR  0x10
#define WZIP_TYPE_METADATA    0x20
#define WZIP_TYPE_TIMESTAMP   0x21
#define WZIP_TYPE_END         0xFF

/* Magic number for container format */
#define WZIP_MAGIC "WZIP"

/* Status codes */
#define WZIP_OK             0
#define WZIP_ERR_NOMEM     -1
#define WZIP_ERR_INVALID   -2
#define WZIP_ERR_BUSY      -3
#define WZIP_ERR_NOT_FOUND -4

/**
 * @brief Compressed sensor payload structure
 */
typedef struct {
    uint8_t  pattern_idx;      /**< Index into pattern dictionary (0-255) */
    int16_t  scale_factor;     /**< Scale factor applied to pattern */
    uint16_t residual_bit_len; /**< Length of residual bitstream in bits */
    uint8_t  residual_padding; /**< Padding bits in last byte */
    uint8_t *residual_bytes;   /**< Golomb-Rice encoded residual */
    uint16_t residual_byte_len;/**< Length of residual in bytes */
} wzip_compressed_payload_t;

/**
 * @brief TLV chunk structure
 */
typedef struct {
    uint8_t  type;        /**< Chunk type identifier */
    uint16_t length;      /**< Payload length in bytes */
    uint8_t *value;       /**< Payload data */
} wzip_tlv_chunk_t;

/**
 * @brief Compression statistics for cloud adaptation
 */
typedef struct {
    uint32_t total_windows;                    /**< Total windows compressed */
    uint32_t pattern_usage[CONFIG_COMPRESSION_CODEBOOK_SIZE]; /**< Per-pattern usage count */
    uint32_t novel_pattern_count;              /**< Number of novel patterns detected */
    uint32_t raw_fallback_count;               /**< Number of raw fallbacks */
    uint32_t total_original_bytes;             /**< Total input bytes */
    uint32_t total_compressed_bytes;           /**< Total output bytes */
} wzip_stats_t;

/**
 * @brief Main compression context
 */
typedef struct {
    /* Configuration */
    uint8_t  num_channels;
    uint16_t window_size;
    uint8_t  golomb_k;
    uint16_t novelty_threshold;
    
    /* Pattern dictionary (pointer to flash) */
    const int16_t (*codebook)[CONFIG_COMPRESSION_WINDOW_SIZE];
    const uint8_t  *codebook_types;
    uint16_t codebook_size;
    
    /* Ring buffer for sensor data */
    int16_t *ring_buffer;
    uint16_t ring_buffer_pos;
    
    /* Output buffer */
    uint8_t *output_buffer;
    uint16_t output_pos;
    uint16_t output_capacity;
    
    /* Statistics */
    wzip_stats_t stats;
    
    /* State */
    bool initialized;
} wzip_context_t;

/**
 * @brief Initialize compression context
 *
 * @param ctx      Compression context (must be pre-allocated)
 * @param codebook Pattern dictionary array [codebook_size][window_size]
 * @param types    Pattern type array [codebook_size]
 * @param size     Number of patterns in codebook
 * @param buffer   Working buffer for ring buffer and output
 * @param buf_size Size of working buffer in bytes
 * @return WZIP_OK on success, error code otherwise
 */
int wzip_init(wzip_context_t *ctx,
              const int16_t (*codebook)[CONFIG_COMPRESSION_WINDOW_SIZE],
              const uint8_t *types,
              uint16_t size,
              uint8_t *buffer,
              uint16_t buf_size);

/**
 * @brief Compress a single sensor window
 *
 * @param ctx     Compression context
 * @param channel Channel index (0-7)
 * @param window  Sensor data window [window_size] samples
 * @return WZIP_OK on success, error code otherwise
 */
int wzip_compress_window(wzip_context_t *ctx, uint8_t channel, const int16_t *window);

/**
 * @brief Compress a block of multi-channel sensor data
 *
 * @param ctx     Compression context
 * @param data    Sensor data array [num_samples][num_channels]
 * @param samples Number of samples
 * @return WZIP_OK on success, error code otherwise
 */
int wzip_compress_block(wzip_context_t *ctx, const int16_t *data, uint16_t samples);

/**
 * @brief Get compressed output data
 *
 * @param ctx    Compression context
 * @param output Output buffer pointer (set to internal buffer)
 * @param len    Output length in bytes
 * @return WZIP_OK on success, error code otherwise
 */
int wzip_get_output(wzip_context_t *ctx, uint8_t **output, uint16_t *len);

/**
 * @brief Reset compression context for new data
 *
 * @param ctx Compression context
 */
void wzip_reset(wzip_context_t *ctx);

/**
 * @brief Get compression statistics
 *
 * @param ctx   Compression context
 * @param stats Statistics structure to fill
 */
void wzip_get_stats(wzip_context_t *ctx, wzip_stats_t *stats);

/**
 * @brief Simple compress function (no context needed)
 *
 * Convenience wrapper that internally manages the compression context.
 * Uses a static internal work buffer configured via Kconfig.
 *
 * @param data      Input sensor data array [samples][num_channels]
 * @param samples   Number of samples
 * @param out_data  Output buffer for compressed data
 * @param out_len   Max output length on input, actual on return
 * @return WZIP_OK on success, error code otherwise
 */
int wzip_compress(const int16_t *data, uint16_t samples,
                  uint8_t *out_data, uint16_t *out_len);

/**
 * @brief Byte-oriented compress (user-friendly wrapper)
 *
 * Takes a raw byte buffer and compresses it. The buffer is interpreted
 * as interleaved int16_t sensor samples in [sample][channel] order.
 * The byte length must be a multiple of (2 * CONFIG_COMPRESSION_NUM_CHANNELS).
 *
 * This is the recommended entry point for callers that work with raw
 * byte buffers (e.g. from a sensor DMA or a packed struct) and do not
 * want to manage int16_t sample counts.
 *
 * @param data      Input byte buffer (interleaved int16 samples)
 * @param data_len  Byte length of input (must be multiple of 2*channels)
 * @param out_data  Output buffer for compressed data
 * @param out_len   Max output length on input, actual on return
 * @return WZIP_OK on success, error code otherwise
 */
int wzip_compress_bytes(const void *data, uint16_t data_len,
                        uint8_t *out_data, uint16_t *out_len);

/**
 * @brief Simple decompress function (uses built-in codebook)
 *
 * Convenience wrapper that uses the codebook from the generated header.
 * No need to pass codebook pointers.
 *
 * @param compressed  Compressed input data
 * @param comp_len    Compressed data length
 * @param output      Output buffer for decompressed data
 * @param output_len  Max output length on input, actual on return
 * @return WZIP_OK on success, error code otherwise
 */
int wzip_decompress(const uint8_t *compressed,
                    uint16_t comp_len,
                    int16_t *output,
                    uint16_t *output_len);

/**
 * @brief Byte-oriented decompress (user-friendly wrapper)
 *
 * Decompresses and returns the raw byte buffer. The output is the
 * interleaved int16_t sensor samples in [sample][channel] order,
 * exactly matching the input format of wzip_compress_bytes().
 *
 * @param compressed  Compressed input data
 * @param comp_len    Compressed data length
 * @param output      Output buffer for decompressed bytes
 * @param output_len  Max output length in BYTES on input, actual on return
 * @return WZIP_OK on success, error code otherwise
 */
int wzip_decompress_bytes(const uint8_t *compressed,
                          uint16_t comp_len,
                          void *output,
                          uint16_t *output_len);

/**
 * @brief Full decompress function (custom codebook)
 *
 * @param codebook    Pattern dictionary
 * @param types       Pattern types
 * @param codebook_size Number of patterns
 * @param compressed  Compressed input data
 * @param comp_len    Compressed data length
 * @param output      Output buffer for decompressed data
 * @param output_len  Max output length on input, actual on return
 * @return WZIP_OK on success, error code otherwise
 */
int wzip_decompress_ex(const int16_t (*codebook)[CONFIG_COMPRESSION_WINDOW_SIZE],
                       const uint8_t *types,
                       uint16_t codebook_size,
                       const uint8_t *compressed,
                       uint16_t comp_len,
                       int16_t *output,
                       uint16_t *output_len);

#ifdef __cplusplus
}
#endif

#endif /* WZIP_COMPRESSION_H */