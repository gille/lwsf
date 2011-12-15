#include <stdlib.h>
#include <stdio.h>

#include "lwsf.h"

#if 1
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif

struct th *p0;
struct th *p1;

void echo(void *arg) {
  struct th *r;
  void *m;
  (void)arg;
  for(;;) {
    m = recv_msg(NULL);
    r = sender(m);
    send_msg(r, &m);
  }
}
 
 void ping(void *arg) {
   for(;;) {
     yield();
   }
 }
 
void pang(void *arg) {
  int i=0;
  printd("head of pang\n");
  for(;;) {
    printd("pang\n");
    yield();
    i++;
    printd("i=%d\n", i);
    if(i == 1000000) {
      exit(0);
    }
  }
}

void pang3(void *arg) {
  int i=0;
  printd("head of pang\n");
  for(;;) {
    printd("pang\n");
    yield();
    i++;
    printd("i=%d\n", i);
    if(i == 3) {
      exit(0);
    }
  }
}

void pang2(void *arg) {
  int i;

  for(i=0; i < 100000; i++) {
    new_thread("p", NULL, NULL); 
  }
  exit(0);
  for(;;) {
    printd("pange\n");
    yield();
    //sleep(1);
  }
}

void handler1(void) {
  p0 = new_thread("p0", pang, NULL);
  p1 = new_thread("p1", pang2, NULL);
  //new_thread("p2", pang, NULL);
}


int main(void) {
  lwsf_start(NULL, handler1);
  return 0;
}
