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

void ping(void *arg) {
  lwsf_mem_cache *m = lwsf_mem_cache_create(1000); 
  int i;

  for(i=0; i < 999; i++)
    lwsf_mem_cache_alloc(m);

  m = lwsf_mem_cache_create(1000); 
  lwsf_mem_cache_free(lwsf_mem_cache_alloc(m));
  exit(lwsf_mem_cache_destroy(m));
  exit(0);
}

void handler1(void) {
  lwsf_thread_new("p1", ping, NULL);
}

int main(void) {
  lwsf_start(NULL, handler1);
  return 0;
}
