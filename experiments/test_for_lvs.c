#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRIDE_MAX 64
#define STRIDE_NEXT 8
#define STRIDE_MIN 8

#define HOPS_MAX 500
#define HOPS_NEXT 10
#define HOPS_MIN 10

#define MEASUREMENT                                                            \
  (((HOPS_MAX - HOPS_MIN) / HOPS_NEXT) *                                       \
   ((STRIDE_MAX - STRIDE_MIN) / STRIDE_NEXT))
#define ITERS 10000

void fill_rand(volatile int *array, int size, int stride, int hops) {
  for (int i = 0; i < hops; i++) {
    array[i * stride] = ((rand() % hops) * stride) % size;
  }
}

void fill_fixed(volatile int *array, int size, int stride, int hops) {
  for (int i = 0; i < hops; i++) {
    array[i * stride] = (((i + 1) * stride) % size);
  }
}

void clear(volatile int *array, int size) {
  for (int i = 0; i < size; i++) {
    array[i] = 0;
  }
}

uint64_t test(volatile int *array, int iters) {
  volatile int dep = 0;

  unsigned long long start = __rdtsc();
  for (int i = 0; i < iters; i++) {
    dep = array[dep];
  }
  unsigned long long end = __rdtsc();
  return end - start;
}

int main(int argc, char *argv[]) {
  uint64_t measurements[2][MEASUREMENT] = {0};

  int i = 0;
  for (int stride = STRIDE_MIN; stride < STRIDE_MAX; stride += STRIDE_NEXT) {
    for (int hops = HOPS_MIN; hops < HOPS_MAX; hops += HOPS_NEXT) {
      int *array = malloc(sizeof(int) * stride * hops);

      // This ensures everything is in cache
      clear(array, stride * hops);
      fill_rand(array, stride * hops, stride, hops);
      measurements[0][i] = test(array, stride * hops);

      // This ensures everything is in cache
      clear(array, stride * hops);
      fill_fixed(array, stride * hops, stride, hops);
      measurements[1][i] = test(array, stride * hops);

      i++;
      free(array);
    }
  }

  FILE *f = fopen("measurements.csv", "w");
  fprintf(f, "stride,hops,random,fixed\n");
  i = 0;
  for (int stride = STRIDE_MIN; stride < STRIDE_MAX; stride += STRIDE_NEXT) {
    for (int hops = HOPS_MIN; hops < HOPS_MAX; hops += HOPS_NEXT) {
      fprintf(f, "%lu,%lu,%lu,%lu\n", stride, hops, measurements[0][i],
              measurements[1][i]);
      i++;
    }
  }

  fclose(f);

  return 0;
}
