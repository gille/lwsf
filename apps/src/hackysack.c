#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "lwsf.h"

#if 0
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif

#define HACKY_SACKERS 10
struct lwsf_th *hackysackers[HACKY_SACKERS];

#define HACKY_SACK 0xBABE
struct hacky_msg {
    uint32_t id;
    uint32_t from;
};

void hackysack(void *arg) {
    uint32_t id = (uint32_t)arg;
    printf("I am thread %d\n", id);
    for(;;) {
	lwsf_thread_yield();
	printf("calling exit!\n");
	exit(0);
    }
}

void handler1(void) {
    struct hacky_msg *msg;
    int i; 
    for(i=0; i < HACKY_SACKERS; i++) 
	hackysackers[i] = lwsf_thread_new("hackysack", hackysack, (void*)i);
    printf("Created %d threads!\n", i);
    msg = lwsf_msg_alloc(sizeof(struct hacky_msg), HACKY_SACK);
    msg->from = -1;
    lwsf_msg_send((void**)&msg, hackysackers[0]);
}


int main(void) {
  lwsf_start(NULL, handler1);
  return 0;
}
