#ifndef CACHE__
#define CACHE__

#include <immintrin.h>
#include <stddef.h>
#include <stdio.h>

#define noinline __attribute__((noinline))

#define CACHE_LINE_SIZE 4096
#define CACHE_HIT_THRESHOLD 80

#define CACHE_LINE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))
#define CACHE_LINE_ALIGNED_PTR __attribute__((aligned(CACHE_LINE_SIZE)))

static inline size_t measure_access_time(char *addr) {
  volatile size_t start = __rdtsc();
  __asm__ __volatile__("movl (%0), %%eax" ::"r"(addr) : "eax");
  _mm_mfence();
  return __rdtsc() - start;
}

static inline void measure_cache(char *probe, size_t *hits) {
  size_t mix_i;
  size_t time;

  for (size_t i = 0; i < 256; i++) {
    mix_i = ((i * 167) + 13) & 255;
    time = measure_access_time(&probe[mix_i * CACHE_LINE_SIZE]);
    if (time <= CACHE_HIT_THRESHOLD) {
      hits[mix_i]++;
    }
  }
}

static inline int valid_read(size_t *hits, size_t len) {
  int i, j, k;
  j = k = -1;
  for (i = 0; i < len; i++) {
    if (j < 0 || hits[i] >= hits[j]) {
      k = j;
      j = i;
    } else if (k < 0 || hits[i] >= hits[k]) {
      k = i;
    }
  }

  if ((hits[j] >= 2 * hits[k] + 5) || (hits[j] == 2 && hits[k] == 0)) {
    return j;
  }

  return -1;
}

#define poors_man_mfence() for (volatile int z = 0; z < 100; z++)

#endif // CACHE__
