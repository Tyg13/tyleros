#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern uint32_t crc32(void *data, uint32_t len);

int main(int argc, char *argv[]) {
  if (argc < 3)
    return EXIT_FAILURE;

  FILE *in = fopen(argv[1], "rb");
  if (!in) {
    perror("opening input failed");
    return EXIT_FAILURE;
  }

  if (fseek(in, 0L, SEEK_END) != 0) {
    perror("fseek() failed");
    return EXIT_FAILURE;
  }
  uint32_t len = ftell(in);
  if (len == 0) {
    return EXIT_FAILURE;
  }
  if (fseek(in, 0L, SEEK_SET) != 0) {
    perror("fseek() failed");
    return EXIT_FAILURE;
  }

  char *data = malloc(len);
  if (!data) {
    fprintf(stderr, "malloc(%d) failed\n", len);
    return EXIT_FAILURE;
  }
  if (fread(data, 1, len, in) != len) {
    if (feof(in))
      fprintf(stderr, "fread(%s) failed: unexpected EOF\n", argv[1]);
    else if (ferror(in)) {
      fprintf(stderr, "fread(%s) failed\n", argv[1]);
      perror("error reading input");
    }
    return EXIT_FAILURE;
  }
  const uint32_t computed_crc32 = crc32(data, len);
  free(data);
  fclose(in);

  FILE *out = fopen(argv[2], "w");
  if (!out) {
    perror("opening output failed");
    return EXIT_FAILURE;
  }
  fprintf(out, "KERNEL_EXPECTED_CRC32 equ 0x%x", computed_crc32);
  fclose(out);

  return EXIT_SUCCESS;
}
