#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "lwsf.h"

#if 0
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif

int HACKY_SACKERS=10;

struct lwsf_th **hackysackers; 

#define HACKY_SACK 0xBABE
#define LOOPS 100

struct hacky_msg {
    uint32_t id;
    uint32_t from;
};

void hackysack(void *arg) {
    uint32_t id = (uint32_t)arg;
    int i;
    struct hacky_msg *msg;
    int to;
    for(i=0; i < LOOPS; i++) {
	msg = lwsf_msg_recv(NULL);
	//printf("I am thread %d got the ball from %d\n", id, msg->from);
	do { 
	    to = rand() % HACKY_SACKERS; 
	} while (to == id); 
	msg->from = id; 
	//printf("Thread %d throw the ball to %d\n", id, to);
	lwsf_msg_send((void**)&msg, hackysackers[to]);       
    }
    printf("I'm done!\n");
    exit(0);
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


int main(int argc, char **argv) {
    if(argc > 1) 
	HACKY_SACKERS=atoi(argv[1]); 
    hackysackers = malloc(sizeof(struct th*)*HACKY_SACKERS); 
    lwsf_start(NULL, handler1);
  return 0;
}
