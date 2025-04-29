#define _GNU_SOURCE
#include "sys/mman.h"
#include <immintrin.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define REPS 1000
// Compare function for qsort
int compare(const void *a, const void *b) {
  return (*(uint64_t *)a > *(uint64_t *)b) - (*(uint64_t *)a < *(uint64_t *)b);
}

double median(uint64_t arr[], int n) {
  qsort(arr, n, sizeof(uint64_t), compare);
  if (n % 2 == 0) {
    return (double)(arr[n / 2 - 1] + arr[n / 2]) / 2.0;
  } else {
    return (double)arr[n / 2];
  }
}

double measure(volatile int *arr, int iters) {
  uint64_t timings[REPS] = {0};
  uint64_t t0, t1;
  volatile register int junk = 0;

  // Dry run
  junk = 0;
  for (int i = 0; i < iters; ++i) {
    junk = arr[junk];
  }

  for (int i = 0; i < REPS; ++i) {
    junk = 0;
    t0 = __rdtsc();
    for (int i = 0; i < iters; ++i) {
      junk = arr[junk];
    }
    t1 = __rdtsc();
    timings[i] = t1 - t0;
  }

  return median(timings, REPS);
}

double measure_unroll(volatile int *arr, int iters) {
  uint64_t timings[REPS] = {0};
  uint64_t t0, t1;
  volatile register int junk = 0;

  // Dry run
  junk = 0;
  for (int i = 0; i < iters; ++i) {
    junk = arr[junk];
  }

  for (int i = 0; i < REPS; ++i) {
    junk = 0;
    t0 = __rdtsc();
#pragma GCC unroll 10
    for (int i = 0; i < iters; ++i) {
      junk = arr[junk];
    }
    t1 = __rdtsc();
    timings[i] = t1 - t0;
  }

  return median(timings, REPS);
}

int main() {
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(0, &mask);
  int result = sched_setaffinity(0, sizeof(mask), &mask);
  if (result == -1) {
    perror("sched_setaffinity");
    return 1;
  }

  const int ITERS_MAX = 1000;
  const int ITERS_MIN = 10;
  const int ITERS_INC = 10;

  const int PAGE_SIZE = 4096;

  srand(time(NULL));

  const int STRIDE = 8;
  FILE *fp = fopen("test_for_splap.csv", "w");
  if (fp == NULL) {
    perror("Failed to open file");
    return 1;
  }

  fprintf(fp, "iters,stride,random_time,random_time_unroll,fixed_time,fixed_"
              "time_unroll\n");
  for (int iters = ITERS_MIN; iters < ITERS_MAX; iters += ITERS_INC) {
    const int NUM_PAGES_BUF = 1 + (iters * STRIDE * sizeof(int)) / PAGE_SIZE;
    int *buf = mmap(NULL, NUM_PAGES_BUF * PAGE_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset((void *)buf, 0, NUM_PAGES_BUF * PAGE_SIZE);

    const int WRITE_MAX = NUM_PAGES_BUF * PAGE_SIZE / sizeof(int);

    for (int i = 0; i < WRITE_MAX; ++i) {
      buf[i] = rand() % WRITE_MAX;
    }

    double random_time = measure(buf, iters);
    double random_time_unroll = measure_unroll(buf, iters);

    int index = 0;
    while (index + STRIDE < WRITE_MAX) {
      buf[index] = index + STRIDE;
      index += STRIDE;
    }

    double fixed_time = measure(buf, iters);
    double fixed_time_unroll = measure_unroll(buf, iters);

    fprintf(fp, "%d,%d,%f,%f,%f,%f\n", iters, STRIDE, random_time,
            random_time_unroll, fixed_time, fixed_time_unroll);

    if (munmap(buf, NUM_PAGES_BUF * PAGE_SIZE) == MAP_FAILED) {
      perror("munmap");
      return 1;
    }
  }

  return 0;
}
