#include <stdlib.h>
#include <stdio.h>

#include "lwsf.h"

#if 1
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif

struct lwsf_th *p0;
struct lwsf_th *p1;

void ping(void *arg) {
   for(;;) {
     lwsf_thread_yield();
   }
 }
 
void pang(void *arg) {
  int i=0;
  printd("head of pang\n");
  for(;;) {
    printd("pang\n");
    lwsf_thread_yield();
    i++;
    printd("i=%d\n", i);
    if(i == 10) {
      exit(0);
    }
  }
}

void pang2(void *arg) {
  for(;;) {
    printd("pange\n");
    lwsf_thread_yield();
    //sleep(1);
  }
}

void handler1(void) {
    int i; 
    for(i=0; i < 500000; i++) 
	p0 = lwsf_thread_new("p0", pang, NULL);
    printf("Created %d threads!\n", i);
    exit(0);
}


int main(void) {
  lwsf_start(NULL, handler1);
  return 0;
}
