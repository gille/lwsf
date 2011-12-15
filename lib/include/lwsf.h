#ifndef mange_g
#define mange_g

struct th;
struct list;
typedef struct list* msg_queue;

struct th* new_thread(const char *name, void (*entry)(void*), void *arg);
void lwsf_start(void (*handler0)(void), void (*handler1)(void));
void yield(void);
void stop_thread(struct th *t);
void lwsf_send_msg(void **_m, struct th *t);
void * lwsf_recv_msg(msg_queue m);
struct th* lwsf_msg_sender(void *m);

#endif
