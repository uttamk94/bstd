/**
 * @file pattern_matcher.h
 * @brief Pattern matching engine for wzip compression
 *
 * Finds the best matching pattern from the dictionary for a given
 * sensor window using L1 distance (Manhattan distance).
 * L1 is preferred over L2 because:
 * - No multiplication needed (faster on MCU)
 * - More robust to outliers
 * - CMSIS-DSP has optimized L1 functions
 */

#ifndef WZIP_PATTERN_MATCHER_H
#define WZIP_PATTERN_MATCHER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pattern match result
 */
typedef struct {
    uint8_t  pattern_idx;   /**< Index of best matching pattern */
    int16_t  scale_factor;  /**< Optimal scale factor */
    int32_t  l1_distance;   /**< L1 distance of best match */
} wzip_match_result_t;

/**
 * @brief Find best matching pattern for a sensor window
 *
 * Searches the codebook for the pattern that minimizes L1 distance
 * after optimal scaling. Only searches patterns matching the
 * specified channel type.
 *
 * @param window       Sensor data window [window_size] samples
 * @param channel      Channel index (0-7)
 * @param codebook     Pattern dictionary [codebook_size][window_size]
 * @param codebook_types Pattern type array [codebook_size]
 * @param codebook_size Number of patterns
 * @param window_size  Number of samples per window
 * @param result       Output: best match result
 * @return 0 on success, -1 if no matching pattern found
 */
int wzip_pattern_find_best(const int16_t *window,
                            uint8_t channel,
                            const int16_t (*codebook)[64],
                            const uint8_t *codebook_types,
                            uint16_t codebook_size,
                            uint16_t window_size,
                            wzip_match_result_t *result);

/**
 * @brief Compute L1 distance between two arrays
 *
 * Uses int32_t accumulator to avoid overflow with 16-bit inputs.
 *
 * @param a First array
 * @param b Second array
 * @param n Number of elements
 * @return L1 distance (sum of absolute differences)
 */
int32_t wzip_l1_distance(const int16_t *a, const int16_t *b, uint16_t n);

/**
 * @brief Compute residual: window - pattern * scale
 *
 * @param window   Original sensor window
 * @param pattern  Pattern from dictionary
 * @param scale    Scale factor
 * @param n        Number of samples
 * @param residual Output residual array
 */
void wzip_compute_residual(const int16_t *window,
                            const int16_t *pattern,
                            int16_t scale,
                            uint16_t n,
                            int32_t *residual);

/**
 * @brief Reconstruct window from pattern and residual
 *
 * @param pattern  Pattern from dictionary
 * @param scale    Scale factor
 * @param residual Residual array
 * @param n        Number of samples
 * @param output   Output reconstructed window
 */
void wzip_reconstruct_window(const int16_t *pattern,
                              int16_t scale,
                              const int32_t *residual,
                              uint16_t n,
                              int16_t *output);

#ifdef __cplusplus
}
#endif

#endif /* WZIP_PATTERN_MATCHER_H */