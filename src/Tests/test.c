
#include <pthread.h>
#include <stdio.h>

int main() {
  int threads = pthread_getconcurrency();
  printf("Number of threads %d\n", threads);
  return 0;
}
