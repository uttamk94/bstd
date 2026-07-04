#ifndef PATTERN_SUB_H
#define PATTERN_SUB_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns a safe output-buffer size in bytes for encoding `len` input bytes.
 *
 * The bound includes the 4-byte original-size header and never underestimates
 * the required output size for the embedded online dictionary.
 *
 * Returns a positive byte count on success and a negative error code on
 * failure.
 */
int encode_buf_bound(int len);

/*
 * Encodes `len` bytes from `data` into `out`.
 *
 * The caller must allocate an output buffer at least `encode_buf_bound(len)`
 * bytes large.
 *
 * On success, returns the encoded byte length.
 * On failure, returns a negative error code.
 *
 * Error codes:
 * - `-1` : invalid argument
 * - `-2` : embedded dictionary error
 * - `-3` : reserved for runtime preparation errors
 * - `-4` : encoding failure
 */
int encode(const void *data, int len, void *out);

/*
 * Returns the output-buffer size in bytes required for decoding `data`.
 *
 * The encoded stream stores the original byte count in its first 4 bytes.
 * This function reads that header without decoding the bitstream.
 *
 * On success, returns the decoded byte length.
 * On failure, returns a negative error code.
 */
int decode_buf_bound(const void *data, int len);

/*
 * Decodes `len` bytes from `data` into `out`.
 *
 * The caller must provide an output buffer large enough for the restored data.
 * The encoded stream stores the original byte count in its 4-byte header.
 *
 * On success, returns the decoded byte length.
 * On failure, returns a negative error code.
 *
 * Error codes:
 * - `-1` : invalid argument
 * - `-2` : embedded dictionary error
 * - `-3` : reserved for runtime preparation errors
 * - `-4` : decoding failure
 */
int decode(const void *data, int len, void *out);

#ifdef __cplusplus
}
#endif

#endif
