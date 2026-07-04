#include "pattern_sub.h"
#include "patterns_online.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PATTERN_FILE_VERSION 1U
#define PATTERN_FILE_HEADER_SIZE 16U
#define PATTERN_FILE_AUX_ONLINE 2U
#define MAX_PATTERN_COUNT 255U
#define MAX_CODE_BITS 255U
#define MAX_CODE_BYTES ((MAX_CODE_BITS + 7U) / 8U)
#define MAX_CODE_STORAGE (MAX_PATTERN_COUNT * MAX_CODE_BYTES)

#define CODEC_ERR_INVALID_ARG (-1)
#define CODEC_ERR_PATTERN_FILE (-2)
#define CODEC_ERR_CACHE (-3)
#define CODEC_ERR_FAILURE (-4)

typedef struct {
  uint32_t pattern_offset;
  uint32_t code_offset;
  uint8_t pattern_len;
  uint8_t code_len_bits;
} EncPatternEntry;

#ifndef PATTERN_SUB_DECODER_ONLY
typedef struct {
  uint8_t *start;
  uint8_t *ptr;
  uint8_t *end;
  uint8_t bit_buffer;
  int bit_count;
} BitWriter;
#endif

#ifndef PATTERN_SUB_ENCODER_ONLY
typedef struct {
  const uint8_t *ptr;
  const uint8_t *end;
  uint8_t bit_buffer;
  int bit_count;
} BitReader;
#endif

typedef struct {
  int ready;
  uint8_t pattern_count;
  uint8_t max_code_len;
  size_t code_bytes;
  EncPatternEntry entries[MAX_PATTERN_COUNT];
  uint8_t code_blob[MAX_CODE_STORAGE];
} PatternRuntime;

static PatternRuntime g_runtime;

static uint32_t read_u32_le(const uint8_t *data) {
  return ((uint32_t)data[0] << 0) | ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static size_t code_num_bytes(uint8_t code_len_bits) {
  return (size_t)(code_len_bits + 7U) / 8U;
}

static int packed_code_bit(const uint8_t *code, uint8_t bit_index) {
  return (code[bit_index / 8] >> (7 - (bit_index % 8))) & 1U;
}

static int bigint_get_bit(const uint8_t *value, int bit_index) {
  size_t byte_from_end = (size_t)bit_index / 8U;
  size_t byte_index = MAX_CODE_BYTES - 1U - byte_from_end;
  return (value[byte_index] >> (bit_index % 8)) & 1U;
}

static int bigint_increment(uint8_t *value) {
  for (size_t i = MAX_CODE_BYTES; i-- > 0;) {
    value[i]++;
    if (value[i] != 0) {
      return 0;
    }
  }
  return -1;
}

static int bigint_shift_left(uint8_t *value, uint8_t shift_bits) {
  for (uint8_t shift = 0; shift < shift_bits; shift++) {
    uint8_t carry = 0;
    for (size_t i = MAX_CODE_BYTES; i-- > 0;) {
      uint8_t next_carry = (uint8_t)((value[i] >> 7) & 1U);
      value[i] = (uint8_t)((value[i] << 1) | carry);
      carry = next_carry;
    }
    if (carry != 0) {
      return -1;
    }
  }
  return 0;
}

static void pack_low_bits(const uint8_t *value, uint8_t bit_count,
                          uint8_t *out) {
  size_t byte_count = code_num_bytes(bit_count);
  memset(out, 0, byte_count);

  for (uint8_t out_bit = 0; out_bit < bit_count; out_bit++) {
    int bit = bigint_get_bit(value, (int)(bit_count - 1U - out_bit));
    if (bit != 0) {
      out[out_bit / 8] |= (uint8_t)(1U << (7 - (out_bit % 8)));
    }
  }
}

static const uint8_t *entry_pattern_data(const EncPatternEntry *entry) {
  return patterns_online_bin + entry->pattern_offset;
}

static const uint8_t *entry_code_data(const EncPatternEntry *entry) {
  return g_runtime.code_blob + entry->code_offset;
}

static int parse_online_dictionary(void) {
  uint32_t version;
  uint32_t count32;
  uint32_t aux_value;
  size_t cursor = PATTERN_FILE_HEADER_SIZE;
  uint8_t max_code_len = 0;
  uint8_t order[MAX_PATTERN_COUNT];
  uint8_t current_code[MAX_CODE_BYTES];
  uint8_t prev_len = 0;
  size_t code_offset = 0;

  if (g_runtime.ready) {
    return 0;
  }

  if (patterns_online_bin_len < PATTERN_FILE_HEADER_SIZE ||
      memcmp(patterns_online_bin, "PATS", 4) != 0) {
    fprintf(stderr, "Invalid embedded pattern dictionary\n");
    return CODEC_ERR_PATTERN_FILE;
  }

  version = read_u32_le(patterns_online_bin + 4);
  count32 = read_u32_le(patterns_online_bin + 8);
  aux_value = read_u32_le(patterns_online_bin + 12);

  if (version != PATTERN_FILE_VERSION || aux_value != PATTERN_FILE_AUX_ONLINE ||
      count32 > MAX_PATTERN_COUNT) {
    fprintf(stderr, "Unsupported embedded pattern dictionary\n");
    return CODEC_ERR_PATTERN_FILE;
  }

  memset(&g_runtime, 0, sizeof(g_runtime));
  for (uint32_t i = 0; i < count32; i++) {
    uint8_t pattern_len;
    uint8_t code_len_bits;

    if (cursor >= patterns_online_bin_len) {
      return CODEC_ERR_PATTERN_FILE;
    }

    pattern_len = patterns_online_bin[cursor++];
    if (pattern_len == 0 || cursor + pattern_len >= patterns_online_bin_len) {
      return CODEC_ERR_PATTERN_FILE;
    }

    g_runtime.entries[i].pattern_offset = (uint32_t)cursor;
    g_runtime.entries[i].pattern_len = pattern_len;
    cursor += pattern_len;

    code_len_bits = patterns_online_bin[cursor++];
    if (code_len_bits == 0) {
      return CODEC_ERR_PATTERN_FILE;
    }

    g_runtime.entries[i].code_len_bits = code_len_bits;
    order[i] = (uint8_t)i;
    if (code_len_bits > max_code_len) {
      max_code_len = code_len_bits;
    }
  }

  if (cursor != patterns_online_bin_len) {
    return CODEC_ERR_PATTERN_FILE;
  }

  for (uint8_t i = 1; i < (uint8_t)count32; i++) {
    uint8_t symbol = order[i];
    uint8_t length = g_runtime.entries[symbol].code_len_bits;
    int insert_at = (int)i - 1;

    while (insert_at >= 0) {
      uint8_t current = order[insert_at];
      uint8_t current_len = g_runtime.entries[current].code_len_bits;
      if (current_len < length || (current_len == length && current < symbol)) {
        break;
      }
      order[insert_at + 1] = current;
      insert_at--;
    }
    order[insert_at + 1] = symbol;
  }

  memset(current_code, 0, sizeof(current_code));
  for (uint8_t order_index = 0; order_index < (uint8_t)count32; order_index++) {
    uint8_t symbol = order[order_index];
    EncPatternEntry *entry = &g_runtime.entries[symbol];
    size_t bytes = code_num_bytes(entry->code_len_bits);

    if (entry->code_len_bits < prev_len || code_offset + bytes > MAX_CODE_STORAGE) {
      return CODEC_ERR_PATTERN_FILE;
    }

    if (order_index != 0) {
      if (bigint_increment(current_code) != 0 ||
          bigint_shift_left(current_code,
                            (uint8_t)(entry->code_len_bits - prev_len)) != 0) {
        return CODEC_ERR_FAILURE;
      }
    }

    entry->code_offset = (uint32_t)code_offset;
    pack_low_bits(current_code, entry->code_len_bits,
                  g_runtime.code_blob + code_offset);
    code_offset += bytes;
    prev_len = entry->code_len_bits;
  }

  g_runtime.pattern_count = (uint8_t)count32;
  g_runtime.max_code_len = max_code_len;
  g_runtime.code_bytes = code_offset;
  g_runtime.ready = 1;
  return 0;
}

#ifndef PATTERN_SUB_DECODER_ONLY
static void init_bit_writer(BitWriter *bw, uint8_t *out, size_t size) {
  bw->start = out;
  bw->ptr = out;
  bw->end = out + size;
  bw->bit_buffer = 0;
  bw->bit_count = 0;
}

static int write_bits(BitWriter *bw, uint32_t bits, int count) {
  for (int i = count - 1; i >= 0; i--) {
    uint8_t bit = (uint8_t)((bits >> i) & 1U);
    if (bw->ptr >= bw->end) {
      return -1;
    }

    if (bit != 0) {
      bw->bit_buffer |= (uint8_t)(1U << (7 - bw->bit_count));
    }
    bw->bit_count++;

    if (bw->bit_count == 8) {
      *bw->ptr++ = bw->bit_buffer;
      bw->bit_buffer = 0;
      bw->bit_count = 0;
    }
  }
  return 0;
}

static int write_packed_code(BitWriter *bw, const EncPatternEntry *entry) {
  const uint8_t *code = entry_code_data(entry);
  for (uint8_t bit = 0; bit < entry->code_len_bits; bit++) {
    if (write_bits(bw, (uint32_t)packed_code_bit(code, bit), 1) != 0) {
      return -1;
    }
  }
  return 0;
}

static int flush_bits(BitWriter *bw) {
  if (bw->bit_count > 0) {
    if (bw->ptr >= bw->end) {
      return -1;
    }
    *bw->ptr++ = bw->bit_buffer;
    bw->bit_buffer = 0;
    bw->bit_count = 0;
  }
  return 0;
}
#endif

#ifndef PATTERN_SUB_ENCODER_ONLY
static uint32_t read_original_size(const uint8_t *data, int size) {
  if (!data || size < 4) {
    return 0;
  }
  return read_u32_le(data);
}

static void init_bit_reader(BitReader *br, const uint8_t *in, size_t size) {
  br->ptr = in;
  br->end = in + size;
  br->bit_buffer = 0;
  br->bit_count = 0;
}

static int read_bit(BitReader *br) {
  if (br->bit_count == 0) {
    if (br->ptr >= br->end) {
      return -1;
    }
    br->bit_buffer = *br->ptr++;
    br->bit_count = 8;
  }

  br->bit_count--;
  return (br->bit_buffer >> br->bit_count) & 1U;
}

static int read_byte(BitReader *br) {
  int value = 0;
  for (int i = 0; i < 8; i++) {
    int bit = read_bit(br);
    if (bit < 0) {
      return -1;
    }
    value = (value << 1) | bit;
  }
  return value;
}
#endif

#ifndef PATTERN_SUB_DECODER_ONLY
static const EncPatternEntry *find_best_match(const uint8_t *input,
                                              size_t remaining) {
  const EncPatternEntry *best = NULL;
  uint8_t best_index = 0;

  for (uint8_t i = 0; i < g_runtime.pattern_count; i++) {
    const EncPatternEntry *entry = &g_runtime.entries[i];
    if ((size_t)entry->pattern_len > remaining) {
      continue;
    }
    if (best && best->pattern_len > entry->pattern_len) {
      continue;
    }
    if (best && best->pattern_len == entry->pattern_len && i > best_index) {
      continue;
    }
    if (memcmp(input, entry_pattern_data(entry), entry->pattern_len) == 0) {
      best = entry;
      best_index = i;
    }
  }

  return best;
}
#endif

#ifndef PATTERN_SUB_DECODER_ONLY
int encode_buf_bound(int len) {
  uint64_t worst_num = 9;
  uint64_t worst_den = 1;
  uint64_t payload_bits;
  uint64_t total_bytes;
  int status;

  if (len < 0) {
    return CODEC_ERR_INVALID_ARG;
  }

  status = parse_online_dictionary();
  if (status != 0) {
    return status;
  }

  for (uint8_t i = 0; i < g_runtime.pattern_count; i++) {
    const EncPatternEntry *entry = &g_runtime.entries[i];
    uint64_t num = (uint64_t)entry->code_len_bits + 1U;
    uint64_t den = (uint64_t)entry->pattern_len;
    if (num * worst_den > worst_num * den) {
      worst_num = num;
      worst_den = den;
    }
  }

  payload_bits = ((uint64_t)(unsigned int)len * worst_num + worst_den - 1U) /
                 worst_den;
  total_bytes = 4U + ((payload_bits + 7U) / 8U);
  if (total_bytes > (uint64_t)INT_MAX) {
    return CODEC_ERR_FAILURE;
  }

  return (int)total_bytes;
}

int encode(const void *data, int len, void *out) {
  const uint8_t *input = (const uint8_t *)data;
  uint8_t *output = (uint8_t *)out;
  size_t offset = 0;
  BitWriter bw;
  int bound;
  int status;

  if ((!data && len > 0) || len < 0 || !out) {
    return CODEC_ERR_INVALID_ARG;
  }

  status = parse_online_dictionary();
  if (status != 0) {
    return status;
  }

  bound = encode_buf_bound(len);
  if (bound < 0) {
    return bound;
  }

  output[0] = (uint8_t)(((uint32_t)len >> 0) & 0xFF);
  output[1] = (uint8_t)(((uint32_t)len >> 8) & 0xFF);
  output[2] = (uint8_t)(((uint32_t)len >> 16) & 0xFF);
  output[3] = (uint8_t)(((uint32_t)len >> 24) & 0xFF);

  if (len == 0) {
    return 4;
  }

  init_bit_writer(&bw, output + 4, (size_t)bound - 4U);

  while (offset < (size_t)len) {
    size_t remaining = (size_t)len - offset;
    const EncPatternEntry *match = find_best_match(input + offset, remaining);

    if (match) {
      if (write_bits(&bw, 1U, 1) != 0 || write_packed_code(&bw, match) != 0) {
        return CODEC_ERR_FAILURE;
      }
      offset += match->pattern_len;
    } else {
      if (write_bits(&bw, 0U, 1) != 0 ||
          write_bits(&bw, input[offset], 8) != 0) {
        return CODEC_ERR_FAILURE;
      }
      offset++;
    }
  }

  if (flush_bits(&bw) != 0) {
    return CODEC_ERR_FAILURE;
  }

  return (int)(4U + (size_t)(bw.ptr - bw.start));
}
#endif

#ifndef PATTERN_SUB_ENCODER_ONLY
int decode_buf_bound(const void *data, int len) {
  if ((!data && len > 0) || len < 4) {
    return CODEC_ERR_INVALID_ARG;
  }

  uint32_t original_size = read_original_size((const uint8_t *)data, len);
  if (original_size > (uint32_t)INT_MAX) {
    return CODEC_ERR_FAILURE;
  }
  return (int)original_size;
}

static int packed_code_equal_prefix(const uint8_t *lhs, const uint8_t *rhs,
                                    uint8_t bit_count) {
  for (uint8_t bit = 0; bit < bit_count; bit++) {
    if (packed_code_bit(lhs, bit) != packed_code_bit(rhs, bit)) {
      return 0;
    }
  }
  return 1;
}

static int decode_next_symbol(BitReader *br) {
  uint8_t prefix[MAX_CODE_BYTES];
  memset(prefix, 0, sizeof(prefix));

  for (uint8_t bit_index = 0; bit_index < g_runtime.max_code_len; bit_index++) {
    int bit = read_bit(br);
    uint8_t current_len = (uint8_t)(bit_index + 1U);

    if (bit < 0) {
      return -1;
    }
    if (bit != 0) {
      prefix[bit_index / 8] |= (uint8_t)(1U << (7 - (bit_index % 8)));
    }

    for (uint8_t symbol = 0; symbol < g_runtime.pattern_count; symbol++) {
      const EncPatternEntry *entry = &g_runtime.entries[symbol];
      if (entry->code_len_bits == current_len &&
          packed_code_equal_prefix(prefix, entry_code_data(entry),
                                   current_len)) {
        return symbol;
      }
    }
  }

  return -1;
}

int decode(const void *data, int len, void *out) {
  const uint8_t *input = (const uint8_t *)data;
  uint8_t *output = (uint8_t *)out;
  uint32_t original_size;
  BitReader br;
  uint8_t *op;
  uint8_t *out_end;
  int status;

  if ((!data && len > 0) || len < 4 || !out) {
    return CODEC_ERR_INVALID_ARG;
  }

  status = parse_online_dictionary();
  if (status != 0) {
    return status;
  }

  original_size = read_original_size(input, len);
  if (original_size > (uint32_t)INT_MAX) {
    return CODEC_ERR_FAILURE;
  }

  init_bit_reader(&br, input + 4, (size_t)len - 4U);
  op = output;
  out_end = output + original_size;

  while (op < out_end) {
    int flag = read_bit(&br);
    if (flag < 0) {
      return CODEC_ERR_FAILURE;
    }

    if (flag == 0) {
      int byte = read_byte(&br);
      if (byte < 0 || op >= out_end) {
        return CODEC_ERR_FAILURE;
      }
      *op++ = (uint8_t)byte;
    } else {
      int symbol = decode_next_symbol(&br);
      const EncPatternEntry *entry;
      if (symbol < 0 || symbol >= g_runtime.pattern_count) {
        return CODEC_ERR_FAILURE;
      }
      entry = &g_runtime.entries[symbol];
      if (op + entry->pattern_len > out_end) {
        return CODEC_ERR_FAILURE;
      }
      memcpy(op, entry_pattern_data(entry), entry->pattern_len);
      op += entry->pattern_len;
    }
  }

  return (int)original_size;
}
#endif
