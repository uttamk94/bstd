/**
 * @file compression.c
 * @brief Main compression engine implementation
 *
 * Orchestrates the full compression pipeline:
 * 1. Ring buffer for sensor data collection
 * 2. Pattern matching via L1 distance
 * 3. Residual computation
 * 4. Golomb-Rice entropy coding
 * 5. TLV container output
 */

#include "compression.h"
#include "codebook.h"
#include "golomb_rice.h"
#include "pattern_matcher.h"
#include "tlv_container.h"
#include <string.h>

int wzip_init(wzip_context_t *ctx,
              const int16_t (*codebook)[CONFIG_COMPRESSION_WINDOW_SIZE],
              const uint8_t *types,
              uint16_t size,
              uint8_t *buffer,
              uint16_t buf_size)
{
    if (ctx == NULL || codebook == NULL || buffer == NULL) {
        return WZIP_ERR_INVALID;
    }
    
    /* Calculate memory split: ring buffer + output buffer */
    uint16_t ring_size = CONFIG_COMPRESSION_WINDOW_SIZE * CONFIG_COMPRESSION_NUM_CHANNELS * sizeof(int16_t);
    uint16_t output_size = buf_size - ring_size;
    
    if (buf_size < ring_size + 64) {
        return WZIP_ERR_NOMEM;
    }
    
    memset(ctx, 0, sizeof(wzip_context_t));
    
    ctx->num_channels = CONFIG_COMPRESSION_NUM_CHANNELS;
    ctx->window_size = CONFIG_COMPRESSION_WINDOW_SIZE;
    ctx->golomb_k = CONFIG_COMPRESSION_GOLOMB_RICE_K;
    ctx->novelty_threshold = CONFIG_COMPRESSION_NOVELTY_THRESHOLD;
    
    ctx->codebook = codebook;
    ctx->codebook_types = types;
    ctx->codebook_size = size;
    
    /* Set up ring buffer (first part of working buffer) */
    ctx->ring_buffer = (int16_t *)buffer;
    ctx->ring_buffer_pos = 0;
    
    /* Set up output buffer (after ring buffer) */
    ctx->output_buffer = buffer + ring_size;
    ctx->output_pos = 0;
    ctx->output_capacity = output_size;
    
    /* Initialize TLV container in output buffer */
    ctx->output_pos = wzip_container_init(ctx->output_buffer, ctx->output_capacity);
    
    ctx->initialized = true;
    
    return WZIP_OK;
}

int wzip_compress_window(wzip_context_t *ctx, uint8_t channel, const int16_t *window)
{
    if (!ctx->initialized) return WZIP_ERR_INVALID;
    if (channel >= ctx->num_channels) return WZIP_ERR_INVALID;
    
    wzip_match_result_t match;
    int ret = wzip_pattern_find_best(
        window,
        channel,
        ctx->codebook,
        ctx->codebook_types,
        ctx->codebook_size,
        ctx->window_size,
        &match
    );
    
    if (ret != 0) {
        /* No matching pattern found */
        ctx->stats.raw_fallback_count++;
        return -1; /* Signal caller to store raw */
    }
    
    /* Check if match is good enough */
    if (match.l1_distance > ctx->novelty_threshold) {
        /* Novel pattern */
        ctx->stats.raw_fallback_count++;
        ctx->stats.novel_pattern_count++;
        return -1; /* Signal caller to store raw */
    }
    
    /* Compute residual */
    int32_t residuals[CONFIG_COMPRESSION_WINDOW_SIZE];
    const int16_t *pattern = ctx->codebook[match.pattern_idx];
    wzip_compute_residual(window, pattern, match.scale_factor, 
                          ctx->window_size, residuals);
    
    /* Encode residuals with Golomb-Rice */
    wzip_golomb_encoder_t golomb;
    wzip_golomb_init(&golomb, ctx->golomb_k);
    
    /* Use a temporary buffer for Golomb-Rice encoding */
    uint8_t temp_buffer[256];
    memset(temp_buffer, 0, sizeof(temp_buffer));
    wzip_bitstream_writer_t writer;
    wzip_writer_init(&writer, temp_buffer, sizeof(temp_buffer));
    
    ret = wzip_golomb_encode_block(&writer, residuals, ctx->window_size, &golomb);
    if (ret != 0) return WZIP_ERR_NOMEM;
    
    /* Flush to get padding */
    uint8_t padding = wzip_writer_flush(&writer);
    uint16_t bytes_written = wzip_writer_bytes_written(&writer);
    
    /* Add compressed sensor chunk to TLV container */
    ret = wzip_container_add_sensor(
        ctx->output_buffer, &ctx->output_pos, ctx->output_capacity,
        channel,
        match.pattern_idx,
        match.scale_factor,
        temp_buffer,
        writer.bit_pos - padding,
        padding
    );
    
    /* Update statistics */
    ctx->stats.total_windows++;
    ctx->stats.pattern_usage[match.pattern_idx]++;
    ctx->stats.total_original_bytes += ctx->window_size * sizeof(int16_t);
    ctx->stats.total_compressed_bytes += bytes_written + 6; /* payload + header */
    
    return ret;
}

int wzip_compress_block(wzip_context_t *ctx, const int16_t *data, uint16_t samples)
{
    if (!ctx->initialized) return WZIP_ERR_INVALID;
    if (data == NULL) return WZIP_ERR_INVALID;
    
    for (uint16_t s = 0; s < samples; s += ctx->window_size) {
        for (uint8_t ch = 0; ch < ctx->num_channels; ch++) {
            int16_t window[CONFIG_COMPRESSION_WINDOW_SIZE];
            
            for (uint16_t w = 0; w < ctx->window_size; w++) {
                uint16_t idx = (s + w) * ctx->num_channels + ch;
                window[w] = (idx < samples * ctx->num_channels) ? data[idx] : 0;
            }
            
            int ret = wzip_compress_window(ctx, ch, window);
    if (ret != 0) {
        /* Fallback: store raw window if compressed path fails */
        ret = wzip_container_add_chunk(
            ctx->output_buffer, &ctx->output_pos, ctx->output_capacity,
            0x10, /* RAW_SENSOR */
            (const uint8_t *)window,
            ctx->window_size * sizeof(int16_t)
        );
        if (ret != 0) return ret; /* Output buffer truly full */
        ctx->stats.raw_fallback_count++;
        continue;
    }
        }
    }
    
    return WZIP_OK;
}

int wzip_get_output(wzip_context_t *ctx, uint8_t **output, uint16_t *len)
{
    if (!ctx->initialized) return WZIP_ERR_INVALID;
    
    *output = ctx->output_buffer;
    *len = ctx->output_pos;
    
    return WZIP_OK;
}

void wzip_reset(wzip_context_t *ctx)
{
    if (!ctx->initialized) return;
    
    ctx->ring_buffer_pos = 0;
    ctx->output_pos = wzip_container_init(ctx->output_buffer, ctx->output_capacity);
    memset(&ctx->stats, 0, sizeof(wzip_stats_t));
}

void wzip_get_stats(wzip_context_t *ctx, wzip_stats_t *stats)
{
    if (ctx == NULL || stats == NULL) return;
    memcpy(stats, &ctx->stats, sizeof(wzip_stats_t));
}

/* Static internal work buffer for the simple wzip_compress() API */
static uint8_t _wzip_static_buffer[4098] __attribute__((aligned(4)));
static bool     _wzip_buffer_used = false;

int wzip_compress(const int16_t *data, uint16_t samples,
                  uint8_t *out_data, uint16_t *out_len)
{
    if (data == NULL || out_data == NULL || out_len == NULL) {
        return WZIP_ERR_INVALID;
    }
    
    /* Import codebook symbols from generated header */
    extern const int16_t codebook[CONFIG_COMPRESSION_CODEBOOK_SIZE][CONFIG_COMPRESSION_WINDOW_SIZE];
    extern const uint8_t codebook_types[CONFIG_COMPRESSION_CODEBOOK_SIZE];
    
    /* Initialize static context once */
    if (!_wzip_buffer_used) {
        wzip_context_t *ctx = (wzip_context_t *)_wzip_static_buffer;
        uint8_t *work_buf = _wzip_static_buffer + sizeof(wzip_context_t);
        uint16_t work_buf_size = sizeof(_wzip_static_buffer) - sizeof(wzip_context_t);
        
        int ret = wzip_init(ctx,
                            codebook,
                            codebook_types,
                            CONFIG_COMPRESSION_CODEBOOK_SIZE,
                            work_buf,
                            work_buf_size);
        if (ret != WZIP_OK) return ret;
        
        _wzip_buffer_used = true;
    }
    
    /* Get context from static buffer */
    wzip_context_t *ctx = (wzip_context_t *)_wzip_static_buffer;
    
    /* Reset for new compression job */
    wzip_reset(ctx);
    
    /* Compress the data block */
    int ret = wzip_compress_block(ctx, data, samples);
    if (ret != WZIP_OK) return ret;
    
    /* Get compressed output */
    uint8_t *compressed;
    uint16_t comp_len;
    ret = wzip_get_output(ctx, &compressed, &comp_len);
    if (ret != WZIP_OK) return ret;
    
    /* Copy to user buffer */
    if (comp_len > *out_len) return WZIP_ERR_NOMEM;
    memcpy(out_data, compressed, comp_len);
    *out_len = comp_len;
    
    return WZIP_OK;
}

int wzip_compress_bytes(const void *data, uint16_t data_len,
                        uint8_t *out_data, uint16_t *out_len)
{
    if (data == NULL || out_data == NULL || out_len == NULL) {
        return WZIP_ERR_INVALID;
    }

    /* Byte length must be a whole number of interleaved int16 samples */
    uint16_t elem_size = sizeof(int16_t) * CONFIG_COMPRESSION_NUM_CHANNELS;
    if (data_len == 0 || (data_len % elem_size) != 0) {
        return WZIP_ERR_INVALID;
    }

    uint16_t samples = data_len / elem_size;
    return wzip_compress((const int16_t *)data, samples, out_data, out_len);
}

int wzip_decompress(const uint8_t *compressed,
                    uint16_t comp_len,
                    int16_t *output,
                    uint16_t *output_len)
{
    /* Import codebook symbols from generated header */
    extern const int16_t codebook[CONFIG_COMPRESSION_CODEBOOK_SIZE][CONFIG_COMPRESSION_WINDOW_SIZE];
    extern const uint8_t codebook_types[CONFIG_COMPRESSION_CODEBOOK_SIZE];
    
    return wzip_decompress_ex(codebook, codebook_types,
                              CONFIG_COMPRESSION_CODEBOOK_SIZE,
                              compressed, comp_len, output, output_len);
}

int wzip_decompress_bytes(const uint8_t *compressed,
                          uint16_t comp_len,
                          void *output,
                          uint16_t *output_len)
{
    if (compressed == NULL || output == NULL || output_len == NULL) {
        return WZIP_ERR_INVALID;
    }

    /* Capacity is in bytes; convert to int16 element count for the core API */
    uint16_t elem_cap = *output_len / sizeof(int16_t);
    if (elem_cap == 0) {
        return WZIP_ERR_INVALID;
    }

    int ret = wzip_decompress(compressed, comp_len, (int16_t *)output, &elem_cap);
    if (ret != WZIP_OK) {
        return ret;
    }

    /* Report actual byte count written */
    *output_len = elem_cap * sizeof(int16_t);
    return WZIP_OK;
}

int wzip_decompress_ex(const int16_t (*codebook)[CONFIG_COMPRESSION_WINDOW_SIZE],
                       const uint8_t *types,
                       uint16_t codebook_size,
                       const uint8_t *compressed,
                       uint16_t comp_len,
                       int16_t *output,
                       uint16_t *output_len)
{
    if (codebook == NULL || compressed == NULL || output == NULL) {
        return WZIP_ERR_INVALID;
    }
    
    uint16_t num_chunks;
    int offset = wzip_container_parse_header(compressed, comp_len, &num_chunks);
    if (offset < 0) return WZIP_ERR_INVALID;
    
    uint16_t out_pos = 0;
    uint16_t max_output = *output_len;
    
    for (uint16_t i = 0; i < num_chunks; i++) {
        uint8_t type;
        const uint8_t *value;
        uint16_t val_len;
        
        int ret = wzip_container_next_chunk(compressed, comp_len, 
                                            (uint16_t *)&offset,
                                            &type, &value, &val_len);
        if (ret != 0) break;
        
        if (type >= 0x01 && type <= 0x08) {
            /* Compressed sensor chunk */
            uint8_t channel = type - 0x01;
            
            if (val_len < sizeof(wzip_compressed_header_t)) continue;
            
            const wzip_compressed_header_t *hdr = 
                (const wzip_compressed_header_t *)value;
            
            if (hdr->pattern_idx >= codebook_size) continue;
            
            /* Get residual data */
            uint16_t residual_bytes = 
                (hdr->residual_bit_len + 7) >> 3;
            
            if (sizeof(wzip_compressed_header_t) + residual_bytes > val_len) {
                continue;
            }
            
            const uint8_t *residual_data = value + sizeof(wzip_compressed_header_t);
            
            /* Decode Golomb-Rice */
            wzip_bitstream_reader_t reader;
            wzip_reader_init(&reader, residual_data, residual_bytes);
            
            /* Read k from bitstream (first 4 bits) */
            uint32_t k_bits = wzip_reader_read_bits(&reader, 4);
            int32_t k = (k_bits > 7) ? 2 : (int32_t)k_bits;
            if (k < 0) continue;
            
            /* Decode residuals */
            int32_t residuals[CONFIG_COMPRESSION_WINDOW_SIZE];
            for (uint16_t r = 0; r < CONFIG_COMPRESSION_WINDOW_SIZE; r++) {
                residuals[r] = wzip_golomb_decode_value(&reader, (uint8_t)k);
            }
            
            /* Reconstruct window */
            const int16_t *pattern = codebook[hdr->pattern_idx];
            int16_t window[CONFIG_COMPRESSION_WINDOW_SIZE];
            
            wzip_reconstruct_window(pattern, hdr->scale_factor,
                                     residuals, CONFIG_COMPRESSION_WINDOW_SIZE,
                                     window);
            
            /* Write to output */
            for (uint16_t w = 0; w < CONFIG_COMPRESSION_WINDOW_SIZE && 
                 out_pos < max_output; w++) {
                /* Interleave channels */
                output[out_pos++] = window[w];
            }
        }
        else if (type == 0x10) {
            /* Raw sensor data */
            if (val_len > 0 && out_pos + val_len / sizeof(int16_t) <= max_output) {
                memcpy(output + out_pos, value, val_len);
                out_pos += val_len / sizeof(int16_t);
            }
        }
        /* Ignore metadata, timestamps, end markers */
    }
    
    *output_len = out_pos;
    return WZIP_OK;
}