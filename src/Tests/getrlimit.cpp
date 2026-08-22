#include <sys/resource.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char** argv) {
  struct rlimit* result;
  result = (rlimit*) malloc(sizeof(struct rlimit));
  memset(result, 0, sizeof(struct rlimit));
  int code = getrlimit(RLIMIT_SIGPENDING, result);
  printf("rlimt_cur: %lu \trlimt_max: %lu \n", (unsigned long)result->rlim_cur, (unsigned long)result->rlim_max);
  free(result);
  return 0;
}
