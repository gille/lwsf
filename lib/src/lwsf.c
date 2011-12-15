#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "lwsf.h"

#include "lwsf_internal.h"

#define THREAD_TYPE_LWSF   0
#define THREAD_TYPE_SYSTEM 1

#define STATE_READY               1
#define STATE_RUNNING             2
#define STATE_BLOCKED             4
#define STATE_BLOCKED_MESSAGE     8

struct  lwsf_msg_queue {
  struct lwsf_list messages;
  struct lwsf_list blocked;
};

struct lwsf_th {
  /* these must be first */
  struct lwsf_list_elem n;
  /* 12 */
  struct lwsf_list_elem n2;
  /* 24 */
  int type;
  /**/
  int state;
  /* 28 */
  char *name;
  /* 32 */
  void *context; 
  lwsf_msg_queue mailbox;

  void (*entry)(void *);
  void *arg;
};

static struct processes {
	struct lwsf_list running;
	struct lwsf_list ready;
	struct lwsf_list blocked;
	struct lwsf_list world;

	struct lwsf_th *current;
} processes;

struct lwsf_msg {
  struct lwsf_list_elem n;
  struct lwsf_th *sender;
  unsigned long aligner;/* fixme */
  unsigned char data[1];
};

struct lwsf_th *current;
struct lwsf_th *idle_thread;

#if 0
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif

extern void SWAP(void *in, void *out); 

void SCHEDULE(void) {
  struct lwsf_th *old, *new;
  printd("schedule called\n");
  if(processes.ready.head == NULL) {
    old = current; 
    new = idle_thread; 
    current = new; 
    SWAP(old->context, new->context); 
  } else {
    if(processes.ready.head != (void*)current) {
      printd("swap to %s\n", ((struct lwsf_th*)LIST_GET_HEAD(&processes.ready))->name);
      old = current; 
      new = ((struct lwsf_th*)LIST_GET_HEAD(&processes.ready)); 
      current = new; 
      printd("swapping %p %p \n", old, new);
      SWAP(old->context, new->context); 
    }
  }
}
void (*hook1)(void) = NULL;

void thread_entry()
{
  static int first = 0;
  struct lwsf_th *th;
  
  if(first == 0) {
    first++;
    
    //printd("calling hook1\n");
    hook1();
    
    while((th = LIST_GET_HEAD(&processes.blocked)) != NULL) {
      LIST_REMOVE_HEAD(&processes.blocked);
	LIST_INSERT_TAIL(&processes.ready, th);
	//printd("Thread %s made ready\n", th->name);
    }
    SCHEDULE();
    /* this is now the idle loop */
    for(;;) {
      printf("IN IDLE!\n");
      exit(1);
      //sleep(1);
      SCHEDULE();
    }
  } 
  printd("calling entry point\n");
  current->entry(current->arg);
  /* kill_thread(); */
}

struct lwsf_th* lwsf_thread_new(const char *name, void (*entry)(void*), void *arg) {
  struct lwsf_th *t; 
  struct lwsf_list_elem *l;
  t = malloc(sizeof(*t));

  l = (struct lwsf_list_elem*)t;
  l->data = t; 
  (l+1)->data = t; 
  t->name = strdup(name);
  LIST_INSERT_TAIL(&processes.world, (l+1));
  LIST_INSERT_TAIL(&processes.blocked, t);
  t->entry = entry;
  t->arg = arg; 
  t->context = (void*)create_context(10*1000);
  t->type = THREAD_TYPE_LWSF;
  t->mailbox.messages.head = t->mailbox.messages.tail = NULL; 
  t->mailbox.blocked.head = t->mailbox.blocked.tail = NULL; 
  t->state = STATE_BLOCKED;
  printd("%s [%p] context created at %p\n", t->name, t, t->context);
  return t;
}

void del_thread(struct lwsf_th *t) {	
	free(t->name);
	LIST_REMOVE_ELEM(&processes.world, t);
	
}

void lwsf_thread_start(struct lwsf_th *t) {
	
	t->state = STATE_READY;

	LIST_REMOVE_ELEM(&processes.blocked, t);
	LIST_INSERT_TAIL(&processes.ready, t);

	SCHEDULE(); 
}

void lwsf_msg_sendq(void **_m, lwsf_msg_queue *dst) {
  struct lwsf_msg *m = *_m;
  *_m = NULL;
  m--;
  m->sender = current; 
  LIST_INSERT_TAIL(&(dst->messages), m);

  /** ... */
  
}

void lwsf_msg_send(void **_m, struct lwsf_th *dst) {
	struct lwsf_msg *m = *_m; 
	*_m = NULL;
	m--;
	m->sender = current; 

	LIST_INSERT_TAIL(&(dst->mailbox.messages), m);	

	if(dst->state == STATE_BLOCKED_MESSAGE) {
	  printd("blocked!\n");
	  LIST_PRINT(&processes.blocked);
	  printd("removed\n");
	  LIST_REMOVE_ELEM(&processes.blocked, dst); 
	  LIST_PRINT(&processes.blocked);
	  LIST_INSERT_TAIL(&processes.ready, dst);
	  SCHEDULE();

	  /* FIXME */
	}
}

void *lwsf_msg_recv_try(lwsf_msg_queue *m) {
  struct lwsf_msg *msg;

  if(m == NULL) {
    m = &current->mailbox;
  }    
  /**/
  if(m->messages.head != NULL) {
    msg = LIST_GET_HEAD(&(m->messages));
    LIST_REMOVE_HEAD(&(m->messages));

    return (msg+1); 
  } else {
    return NULL;
  }
}

void * lwsf_msg_recv(lwsf_msg_queue *m) {
  struct lwsf_msg *msg;

  /* blah? */
  while((msg = lwsf_msg_recv_try(m)) == NULL) {
    LIST_REMOVE_HEAD(&processes.ready);
    LIST_INSERT_TAIL(&processes.blocked, current); 
    LIST_PRINT(&processes.blocked);
    current->state = STATE_BLOCKED_MESSAGE;
    SCHEDULE();
  } 
  
  return (msg); 
}

struct lwsf_th* lwsf_msg_sender(void *m) { 
  struct lwsf_msg *msg = m;
  msg--;
  return msg->sender;
}

void stop_thread(struct lwsf_th *t) { 
  if(t == current) {
    printd("stopping current thread\n");
    LIST_REMOVE_HEAD(&processes.ready); 
    LIST_INSERT_TAIL(&processes.blocked, t); 
    SCHEDULE();
  } else {
    /* find the fucker */
  }
}

void lwsf_thread_yield(void) {
  printd("yield called\n");
  //  LIST_REMOVE_ELEM(&processes.ready, current);
  LIST_REMOVE_HEAD(&processes.ready); 
  LIST_INSERT_TAIL(&processes.ready, current);
  if(current != (struct lwsf_th*)(processes.ready.head)){ 
    SCHEDULE();
  }
}
 
 

void add_fd() {
}


void read(){}

void lwsf_start(void (*handler0)(void), void (*handler1)(void)) 
{
  struct lwsf_th *th;
  void *never_used; 
  if(handler0)
    handler0();
  hook1 = handler1;
  th = lwsf_thread_new("idle", NULL, NULL);
  idle_thread = th;
  current = th;
  /* Remove idle from blocked threads */
  LIST_REMOVE_HEAD(&processes.blocked);


  SWAP(&never_used, idle_thread->context);

  //never reached  SCHEDULE();
}

/* There's no point in receiving data if no thread is available to process it */

void *lwsf_msg_alloc(int size, int id) {
  uint32_t *idp;
  struct lwsf_msg *m = (struct lwsf_msg*)malloc(sizeof(struct lwsf_msg) + size); 
  m->sender = NULL;
  m->n.data = m;
  m++;
  idp = (uint32_t*)m;
 *idp = id;

 return m;
}

lwsf_msg_queue* lwsf_msgq_create() {
  int s= sizeof(lwsf_msg_queue);
  lwsf_msg_queue *m = malloc(s);
  
  memset(m, 0, sizeof(*m)); 
  return m;
}

void print_state(){ 
  struct lwsf_th *t;
  for(t = (struct lwsf_th*)processes.world.head; t != NULL; t = (struct lwsf_th*)t->n2.next) {
    printd("process %s\n", t->name);
  }
}
