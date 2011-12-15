#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "lwsf.h"

#define THREAD_TYPE_LWSF   0
#define THREAD_TYPE_SYSTEM 1

#define STATE_READY               1
#define STATE_RUNNING             2
#define STATE_BLOCKED             4
#define STATE_BLOCKED_MESSAGE     8


struct th {
  /* these must be first */
  struct list_elem n;
  /* 12 */
  struct list_elem n2;
  /* 24 */
  int type;
  /**/
  int state;
  /* 28 */
  char *name;
  /* 32 */
  void *context; 
  struct list mailbox;

  void (*entry)(void *);
  void *arg;
};

static struct processes {
	struct list running;
	struct list ready;
	struct list blocked;
	struct list world;

	struct th *current;
} processes;

struct lwsf_msg {
  struct list_elem n;
  struct th *sender;
  unsigned long aligner;/* fixme */
  unsigned char data[1];
};

struct th *current;
struct th *idle_thread;

#if 0
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif

extern void SWAP(void *in, void *out); 

void SCHEDULE(void) {
  struct th *old, *new;
  printd("schedule called\n");
  if(processes.ready.head == NULL) {
    old = current; 
    new = idle_thread; 
    current = new; 
    SWAP(old->context, new->context); 
  } else {
    if(processes.ready.head != (void*)current) {
      printd("swap to %s\n", ((struct th*)LIST_GET_HEAD(&processes.ready))->name);
      old = current; 
      new = ((struct th*)LIST_GET_HEAD(&processes.ready)); 
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
  struct th *th;
  
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

struct th* new_thread(const char *name, void (*entry)(void*), void *arg) {
  struct th *t; 
  struct list_elem *l;
  t = malloc(sizeof(*t));

  l = (struct list_elem*)t;
  l->data = t; 
  (l+1)->data = t; 
  t->name = strdup(name);
  LIST_INSERT_TAIL(&processes.world, (l+1));
  LIST_INSERT_TAIL(&processes.blocked, t);
  t->entry = entry;
  t->arg = arg; 
  t->context = (void*)create_context(10*1000);
  t->type = THREAD_TYPE_LWSF;
  t->mailbox.head = t->mailbox.tail = NULL; 
  t->state = STATE_BLOCKED;
  printd("%s [%p] context created at %p\n", t->name, t, t->context);
  return t;
}

void del_thread(struct th *t) {	
	free(t->name);
	LIST_REMOVE_ELEM(&processes.world, t);
	
}

void start_thread() {
	struct th *t;
	t->state = STATE_RUNNING;

	LIST_REMOVE_ELEM(&processes.blocked, t);
	LIST_INSERT_TAIL(&processes.ready, t);

	SCHEDULE(); 
}

void lwsf_msg_send(void **_m, struct th *dst) {
	struct lwsf_msg *m = *_m; 
	*_m = NULL;
	m--;
	m->sender = current; 
	LIST_INSERT_TAIL(&dst->mailbox, m);	
	
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


void * lwsf_msg_recv(msg_queue m) {
  struct lwsf_msg *msg;

  if(m == NULL) {
    m = &current->mailbox;
    
    /**/
    if(m->head) {
      printd("HEAD!\n");
      LIST_REMOVE_HEAD(m);
    } else {
      printd("Remove from processes!\n");
      LIST_REMOVE_HEAD(&processes.ready);
      LIST_INSERT_TAIL(&processes.blocked, current); 
      LIST_PRINT(&processes.blocked);
      current->state = STATE_BLOCKED_MESSAGE;
      SCHEDULE();
      
      printd("Got out!\n");
      msg = LIST_GET_HEAD(m);
      LIST_REMOVE_HEAD(m);
     
      return (msg+1); 
    }
  } else {
    printd("Ouch I don't support msg queues just yet\n");
    exit(0);
  }
}

struct th* lwsf_msg_sender(void *m) { 
  struct lwsf_msg *msg = m;
  msg--;
  return msg->sender;
}

void stop_thread(struct th *t) { 
  if(t == current) {
    printd("stopping current thread\n");
    LIST_REMOVE_HEAD(&processes.ready); 
    LIST_INSERT_TAIL(&processes.blocked, t); 
    SCHEDULE();
  } else {
    /* find the fucker */
  }
}

void yield(void) {
  printd("yield called\n");
  //  LIST_REMOVE_ELEM(&processes.ready, current);
  LIST_REMOVE_HEAD(&processes.ready); 
  LIST_INSERT_TAIL(&processes.ready, current);
  if(current != (struct th*)(processes.ready.head)){ 
    SCHEDULE();
  }
}
 
 

void add_fd() {
}


void read(){}

void lwsf_start(void (*handler0)(void), void (*handler1)(void)) 
{
  struct th *th;
  void *never_used; 
  if(handler0)
    handler0();
  hook1 = handler1;
  th = new_thread("idle", NULL, NULL);
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

msg_queue lwsf_msgq_create() {
  int s= sizeof(struct list);
  msg_queue m = malloc(s);
  
  m->head = m->tail = NULL; 
  return m;
}

void print_state(){ 
  struct th *t;
  for(t = (struct th*)processes.world.head; t != NULL; t = (struct th*)t->n2.next) {
    printd("process %s\n", t->name);
  }
}
