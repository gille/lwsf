#ifndef mange_g
#define mange_g

struct th;
struct list;
typedef struct list msg_queue;

struct th* new_thread(const char *name, void (*entry)(void*), void *arg);
void lwsf_start(void (*handler0)(void), void (*handler1)(void));
void yield(void);
void stop_thread(struct th *t);
void lwsf_msg_send(void **_m, struct th *t);
void lwsf_msg_sendq(void **_m, msg_queue *t);
void * lwsf_msg_recv(msg_queue * m);
void *lwsf_msg_recv_try(msg_queue *m);
struct th* lwsf_msg_sender(void *m);
void *lwsf_msg_alloc(int size, int id);
msg_queue* lwsf_msgq_create();

#endif
