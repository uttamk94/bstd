/**
 * @file pattern_matcher.c
 * @brief Pattern matching engine implementation
 *
 * Uses L1 distance for pattern matching, optimized for Cortex-M4+.
 * No floating-point, no division (except shifts).
 */

#include "pattern_matcher.h"
#include <stddef.h>

int32_t wzip_l1_distance(const int16_t *a, const int16_t *b, uint16_t n)
{
    int32_t sum = 0;
    for (uint16_t i = 0; i < n; i++) {
        int32_t diff = (int32_t)a[i] - (int32_t)b[i];
        sum += (diff < 0) ? -diff : diff;
    }
    return sum;
}

void wzip_compute_residual(const int16_t *window,
                            const int16_t *pattern,
                            int16_t scale,
                            uint16_t n,
                            int32_t *residual)
{
    for (uint16_t i = 0; i < n; i++) {
        residual[i] = (int32_t)window[i] - ((int32_t)pattern[i] * scale);
    }
}

void wzip_reconstruct_window(const int16_t *pattern,
                              int16_t scale,
                              const int32_t *residual,
                              uint16_t n,
                              int16_t *output)
{
    for (uint16_t i = 0; i < n; i++) {
        int32_t val = ((int32_t)pattern[i] * scale) + residual[i];
        /* Clamp to int16 range */
        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        output[i] = (int16_t)val;
    }
}

int wzip_pattern_find_best(const int16_t *window,
                            uint8_t channel,
                            const int16_t (*codebook)[64],
                            const uint8_t *codebook_types,
                            uint16_t codebook_size,
                            uint16_t window_size,
                            wzip_match_result_t *result)
{
    int32_t best_dist = 0x7FFFFFFF;
    uint8_t best_idx = 0;
    int16_t best_scale = 1;
    int found = 0;
    
    for (uint16_t i = 0; i < codebook_size; i++) {
        if (codebook_types[i] != channel) {
            continue;
        }
        
        const int16_t *pattern = codebook[i];
        
        /* Compute mean ratio for scale factor (avoid division) */
        int32_t window_sum = 0;
        int32_t pattern_sum = 0;
        for (uint16_t j = 0; j < window_size; j++) {
            window_sum += window[j];
            pattern_sum += pattern[j];
        }
        
        int16_t scale = 1;
        if (pattern_sum != 0) {
            int32_t mean_pattern = pattern_sum / window_size;
            if (mean_pattern != 0) {
                /* Approximate scale as ratio of sums */
                scale = (int16_t)(window_sum / mean_pattern);
            }
        }
        if (scale < 1) scale = 1;
        if (scale > 100) scale = 100; /* Sanity limit */
        
        /* Compute L1 distance with scaling */
        int32_t dist = 0;
        for (uint16_t j = 0; j < window_size; j++) {
            int32_t predicted = (int32_t)pattern[j] * scale;
            int32_t diff = (int32_t)window[j] - predicted;
            dist += (diff < 0) ? -diff : diff;
        }
        
        if (dist < best_dist) {
            best_dist = dist;
            best_idx = (uint8_t)i;
            best_scale = scale;
            found = 1;
        }
    }
    
    if (!found) {
        return -1;
    }
    
    result->pattern_idx = best_idx;
    result->scale_factor = best_scale;
    result->l1_distance = best_dist;
    
    return 0;
}