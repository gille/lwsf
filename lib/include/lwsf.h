#ifndef mange_g
#define mange_g

struct lwsf_th;
struct lwsf_list;
struct lwsf_msg_queue;

typedef struct lwsf_msg_queue lwsf_msg_queue;
typedef struct lwsf_th lwsf_th;

extern void lwsf_start(void (*handler0)(void), void (*handler1)(void));

extern lwsf_th* lwsf_thread_new(const char *name, void (*entry)(void*), void *arg);
extern void lwsf_thread_start(lwsf_th *t);
extern void lwsf_thread_yield(void);
extern void lwsf_thread_stop(lwsf_th *t);

extern void lwsf_msg_send(void **_m, lwsf_th *t);
extern void lwsf_msg_sendq(void **_m, lwsf_msg_queue *t);
extern void * lwsf_msg_recv(lwsf_msg_queue * m);
extern void *lwsf_msg_recv_try(lwsf_msg_queue *m);
extern lwsf_th* lwsf_msg_sender(void *m);
extern void *lwsf_msg_alloc(int size, int id);
extern void lwsf_msg_free(void **m);

extern lwsf_msg_queue* lwsf_msgq_create(void);

#endif
