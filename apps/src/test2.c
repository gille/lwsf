#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "lwsf.h"
#if 0
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif


void echo(void *arg) {
  uint32_t *m;
  for(;;) {
    printd("Waiting for message\n");
    m = lwsf_msg_recv(NULL);
    printd("Received message! %p\n", m);
    printd("msg_no: %d\n", *m);
    lwsf_msg_send((void**)&m, lwsf_msg_sender(m));
  }
}

void ping(void *arg) {
  struct th *t = arg;
  int i;
  uint32_t *m;
  m = lwsf_msg_alloc(10, 10);
  for(i=0; i < 5; i++) {
    printd("sent message %p\n", m);
    lwsf_msg_send((void**)&m, t);
    m = lwsf_msg_recv(NULL);
    printd("Received message! %p\n", m);
    if(*m != 10) {
      printd("wrong message number\n");
      exit(1);
    }
  }
  exit(0);
}

void handler1(void) {
  struct th *t;
  t = new_thread("p0", echo, NULL);
  new_thread("p1", ping, t);
}

int main(void) {
  lwsf_start(NULL, handler1);
  return 0;
}
