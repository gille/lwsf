#include <assert.h>

#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "lwsf.h"

#include "lwsf_internal.h"
#include "arch_internal.h"

#define THREAD_TYPE_LWSF   0
#define THREAD_TYPE_SYSTEM 1


#define STATE_RUNNING             0
#define STATE_READY               1

#define STATE_BLOCKED_MESSAGE     4
#define STATE_BLOCKED             8

#define STATE_STOPPED             16

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
  void *stack;

  unsigned char context[LWSF_ARCH_CONTEXT_SIZE];
  lwsf_msg_queue mailbox;

  void (*entry)(void *);
  void *arg;
};

static struct lwsf_world {
	struct lwsf_list running;
	struct lwsf_list ready;
	struct lwsf_list blocked;
	struct lwsf_list stopped;
	struct lwsf_list world;

	struct lwsf_th *current;

  /* FIXME */
  int orig_state[16];
} lwsf_world;

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

void SCHEDULE(void) {
  struct lwsf_th *old, *new;
  printd("schedule called\n");
  if(lwsf_world.ready.head == NULL) {
    old = current; 
    new = idle_thread; 
    current = new; 
    lwsf_arch_thread_swap(old->context, new->context); 
  } else {
    if(lwsf_world.ready.head != (void*)current) {
      printd("swap to %s\n", ((struct lwsf_th*)LIST_GET_HEAD(&lwsf_world.ready))->name);
      old = current; 
      new = ((struct lwsf_th*)LIST_GET_HEAD(&lwsf_world.ready)); 
      current = new; 
      printd("swapping %p %p \n", old, new);
      lwsf_arch_thread_swap(old->context, new->context); 
    }
  }
}

void lwsf_idle_thread(void * arg) {
  struct lwsf_th *th;
  void (*handler1)(void) = arg;

  handler1(); 
  
  while((th = LIST_GET_HEAD(&lwsf_world.blocked)) != NULL) {
    LIST_REMOVE_HEAD(&lwsf_world.blocked);
    LIST_INSERT_TAIL(&lwsf_world.ready, th);
    //printd("Thread %s made ready\n", th->name);
  }
  SCHEDULE();
  for(;;) {
    if(lwsf_world.world.head == lwsf_world.world.tail) {
      /* No processes exist! */
      printf("I'm out of here!\n");
      lwsf_arch_thread_swap(idle_thread->context, lwsf_world.orig_state);
    }
    printf("IN IDLE!\n");
    exit(1);
    //sleep(1);
    SCHEDULE();        
  }
}

void lwsf_thread_entry(void) {
  printd("calling entry point\n");
  current->entry(current->arg);
  lwsf_thread_kill(current);
  /* never reached */
}

struct lwsf_th* lwsf_thread_new(const char *name, void (*entry)(void*), void *arg) {
  struct lwsf_th *t; 
  struct lwsf_list_elem *l;
  int stack_size = 100*1000;
  void *stack;

  t = malloc(sizeof(*t));
  if(t == NULL) 
    goto out;
  stack = malloc(stack_size);
  if(stack == NULL) 
    goto out_stack;
  l = (struct lwsf_list_elem*)t;
  l->data = t; 
  (l+1)->data = t; 
  t->name = strdup(name);
  if(t->name == NULL) 
    goto out_name;
  LIST_INSERT_TAIL(&lwsf_world.world, (l+1));
  LIST_INSERT_TAIL(&lwsf_world.blocked, t);
  t->entry = entry;
  t->arg = arg; 
  lwsf_arch_create_context(t->context, stack, stack_size);
  t->stack = stack;
  t->type = THREAD_TYPE_LWSF;
  t->mailbox.messages.head = t->mailbox.messages.tail = NULL; 
  t->mailbox.blocked.head = t->mailbox.blocked.tail = NULL; 
  t->state = STATE_BLOCKED;
  printd("%s [%p] context created at %p\n", t->name, t, t->context);
  return t;

 out_name:
  free(stack);
 out_stack:
  free(t);
 out:
  return NULL;
}

void lwsf_thread_delete(struct lwsf_th *t) {	
	LIST_REMOVE_ELEM(&lwsf_world.world, &t->n2);
	free(t->name);
	free(t->stack);
	free(t);	
}

void lwsf_thread_kill(struct lwsf_th *t) {
  if(t == current) {
    LIST_REMOVE_ELEM(&lwsf_world.ready, t);
  } else {
    exit(-1);
  }
  LIST_REMOVE_ELEM(&lwsf_world.world, &(t->n2));
  free(t->name);
  free(t);

  /* FIXME: Free context! */
  SCHEDULE();
  
}

void lwsf_thread_start(struct lwsf_th *t) {	
	t->state = STATE_READY;

	LIST_REMOVE_ELEM(&lwsf_world.blocked, t);
	LIST_INSERT_TAIL(&lwsf_world.ready, t);

	SCHEDULE(); 
}

void lwsf_msg_sendq(void **_m, lwsf_msg_queue *dst) {
  struct lwsf_msg *m = *_m;
  *_m = NULL;
  m--;
  m->sender = current; 
  LIST_INSERT_TAIL(&(dst->messages), m);

  if(dst->blocked.head != NULL) {
    struct lwsf_th *t = LIST_GET_HEAD(&(dst->blocked));
    LIST_REMOVE_HEAD(&(dst->blocked)); 
    LIST_REMOVE_ELEM(&(lwsf_world.blocked), t);
    printd("Process %p is out and ready!!\n", t);
    LIST_INSERT_TAIL(&(lwsf_world.ready), t);
    SCHEDULE();
  }
}

void lwsf_msg_send(void **_m, struct lwsf_th *dst) {
	struct lwsf_msg *m = *_m; 
	*_m = NULL;
	m--;
	m->sender = current; 

	LIST_INSERT_TAIL(&(dst->mailbox.messages), m);	

	if(dst->state == STATE_BLOCKED_MESSAGE) {
	  printd("blocked!\n");
	  LIST_PRINT(&lwsf_world.blocked);
	  
	  printd("removed\n");
	  LIST_REMOVE_ELEM(&lwsf_world.blocked, dst); 
	  LIST_PRINT(&lwsf_world.blocked);
	  LIST_INSERT_TAIL(&lwsf_world.ready, dst);
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
    LIST_REMOVE_HEAD(&lwsf_world.ready);
    LIST_INSERT_TAIL(&lwsf_world.blocked, current); 
    LIST_PRINT(&lwsf_world.blocked);
    current->state = STATE_BLOCKED_MESSAGE;

    if(m != NULL) {
      LIST_INSERT_TAIL(&(m->blocked), current);
    }
    SCHEDULE();
  } 
  
  return (msg); 
}

struct lwsf_th* lwsf_msg_sender(void *m) { 
  struct lwsf_msg *msg = m;
  msg--;
  return msg->sender;
}

void lwsf_thread_stop(struct lwsf_th *t) { 
  if(t == current) {
    printd("stopping current thread\n");
    LIST_REMOVE_HEAD(&lwsf_world.ready); 
    LIST_INSERT_TAIL(&lwsf_world.blocked, t); 
    SCHEDULE();
  } else {
    if(t->state <= STATE_READY) {
      LIST_REMOVE_ELEM(&lwsf_world.ready, t);
      LIST_INSERT_TAIL(&lwsf_world.stopped, t);
      t->state= STATE_STOPPED;
    } else {
      if(t->state <= STATE_BLOCKED) {
	LIST_REMOVE_ELEM(&lwsf_world.blocked, t);
	LIST_INSERT_TAIL(&lwsf_world.stopped, t);
      }
    }    
    /* it's already stopped or blocked? */
  }
}

void lwsf_thread_yield(void) {
  printd("yield called\n");
  //  LIST_REMOVE_ELEM(&lwsf_world.ready, current);
  LIST_REMOVE_HEAD(&lwsf_world.ready); 
  LIST_INSERT_TAIL(&lwsf_world.ready, current);
  if(current != (struct lwsf_th*)(lwsf_world.ready.head)){ 
    SCHEDULE();
  }
}

void lwsf_start(void (*handler0)(void), void (*handler1)(void)) 
{
  if(handler0)
    handler0();

  idle_thread = lwsf_thread_new("idle", lwsf_idle_thread, handler1);
  assert(idle_thread != NULL);
  current = idle_thread;
  /* Remove idle from blocked threads */
  LIST_REMOVE_HEAD(&lwsf_world.blocked);

  printd("swapping contexts\n");
  lwsf_arch_thread_swap(lwsf_world.orig_state, idle_thread->context);
  lwsf_thread_delete(idle_thread);
  LIST_PRINT(&lwsf_world.world);
  idle_thread = NULL;
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
  
  LIST_INIT(&m->messages);
  LIST_INIT(&m->blocked);

  return m;
}

void print_state(){ 
  struct lwsf_th *t;
  for(t = (struct lwsf_th*)lwsf_world.world.head; t != NULL; t = (struct lwsf_th*)t->n2.next) {
    printd("process %s\n", t->name);
  }
}
