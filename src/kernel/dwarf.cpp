#include "dwarf.h"
#include "libadt/optional.h"
#include <stdint.h>

typedef struct {
  uint32_t length;
  uint16_t version;
  uint32_t header_length;
  uint8_t min_instruction_length;
  uint8_t default_is_stmt;
  int8_t line_base;
  uint8_t line_range;
  uint8_t opcode_base;
  uint8_t std_opcode_lengths[12];
} __attribute__((packed)) debug_line_header;

adt::optional<uint64_t> decode_leb128(char *data, unsigned *len) {
  uint64_t result = 0;
  unsigned i = 0;
  do {
    char input = data[i];
    result |= ((uint64_t)input & ~0x80) << (8 * i);
    if ((data[i] & 0x80) == 0)
      break;

    if (++i >= 8)
      return adt::none;
  } while (true);

  *len = i;
  return result;
}
