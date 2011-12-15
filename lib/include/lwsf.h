#ifndef mange_g
#define mange_g

struct lwsf_th;
struct lwsf_list;
struct lwsf_msg_queue;

typedef struct lwsf_msg_queue lwsf_msg_queue;

void lwsf_start(void (*handler0)(void), void (*handler1)(void));

struct lwsf_th* lwsf_thread_new(const char *name, void (*entry)(void*), void *arg);
void lwsf_thread_start(struct lwsf_th *t);
void lwsf_thread_yield(void);
void lwsf_thread_stop(struct lwsf_th *t);

void lwsf_msg_send(void **_m, struct lwsf_th *t);
void lwsf_msg_sendq(void **_m, lwsf_msg_queue *t);
void * lwsf_msg_recv(lwsf_msg_queue * m);
void *lwsf_msg_recv_try(lwsf_msg_queue *m);
struct lwsf_th* lwsf_msg_sender(void *m);
void *lwsf_msg_alloc(int size, int id);

lwsf_msg_queue* lwsf_msgq_create();

#endif
