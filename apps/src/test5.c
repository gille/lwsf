#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "lwsf.h"
#include "lwsf_mem_cache.h"
#if 0
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif

void handler1(void) {
  lwsf_thread_new("p1", ping, NULL);
}

int main(void) {
  lwsf_start(NULL, handler1);
  return 0;
}
