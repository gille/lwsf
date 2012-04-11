#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "lwsf.h"
#include "lwsf_mem_cache.h"
#include "lwsf_internal.h"

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
  void *context; 
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

    struct lwsf_mem_cache *thread_mc;
} lwsf_world;

struct lwsf_msg {
  struct lwsf_list_elem n;
  struct lwsf_th *sender;
  unsigned long aligner;/* fixme */
  unsigned char data[1];
};

struct lwsf_th *current;
struct lwsf_th *idle_thread;

#if 1
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif

#ifdef ASSERT_BUILD
static int is_in(struct lwsf_list *l, void *d) {
    struct lwsf_list_elem *n;
    if(l == NULL) 
	return 0;
    for(n = l->head; n != NULL; n=n->next) 
	if(n->data == d)
	    return 1;
    
    return 0;

}

static void ASSERT_WORLD(void) 
{
    int i=0;
    struct lwsf_list_elem *n;
    for(n=lwsf_world.world.head; n != NULL; n=n->next)
	i++;
    //printf("There are %d threadlets\n", i);
    for(n=lwsf_world.world.head; n != NULL; n=n->next)
	if(!((is_in(&lwsf_world.blocked, n->data)) ||is_in(&lwsf_world.ready, n->data) || (n->data+12) != idle_thread)) {
	    printf("A threadlet has been lost! %p\n", n->data);
	    printf("%p\n", idle_thread);
	    printf("A threadlet has been lost! %#x\n", ((unsigned long)n)-12);
	    exit(-1);
	}
}
#endif

static void SCHEDULE(void) {
    struct lwsf_th *old, *new;
    printd("schedule called\n");
#ifdef ASSERT_BUILD
    ASSERT_WORLD();
#endif
    if(lwsf_world.ready.head == NULL) {
	printd("ready_head is NULL swap to idle\n");
	old = current; 
	new = idle_thread; 
	current = new; 
	lwsf_arch_thread_swap(old->context, new->context); 
    } else {
	printd("ready_head is: %p %p\n", lwsf_world.ready.head, current);
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

void lwsf_thread_entry(void) {
  static int first = 0;
  struct lwsf_th *th;
  void (*handler1)(void);
  printd("here\n");
  if(first == 0) {
    first++;
    handler1 = (void*)current->entry; 
    //printd("calling hook1\n");
    handler1();
    //LIST_PRINT(&lwsf_world.blocked); 
    while((th = LIST_GET_HEAD(&lwsf_world.blocked)) != NULL) {
	printd("Make a thread ready!\n");
	LIST_REMOVE_HEAD(&lwsf_world.blocked);
	LIST_INSERT_TAIL(&lwsf_world.ready, th);
	//printd("Thread %s made ready\n", th->name);
    }
    SCHEDULE();
    /* this is now the idle loop */
    for(;;) {
	printf("IN IDLE!\n");
	LIST_PRINT(&lwsf_world.blocked);
	LIST_PRINT(&lwsf_world.ready);
	//exit(1);
      sleep(1);
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
  //t = lwsf_mem_cache_alloc(lwsf_world.thread_mc); /* FIXME: broken! */
  l = (struct lwsf_list_elem*)t;
  l->data = t; 
  (l+1)->data = t; 
  t->name = strdup(name);
  LIST_INSERT_TAIL(&lwsf_world.world, (l+1));
  LIST_INSERT_TAIL(&lwsf_world.blocked, t);
  t->entry = entry;
  t->arg = arg; 
  t->context = (void*)lwsf_arch_create_context(1000*100);
  t->type = THREAD_TYPE_LWSF;
  t->mailbox.messages.head = t->mailbox.messages.tail = NULL; 
  t->mailbox.blocked.head = t->mailbox.blocked.tail = NULL; 
  t->state = STATE_BLOCKED;
  printd("%s [%p] context created at %p\n", t->name, t, t->context);
  return t;
}

void lwsf_thread_delete(struct lwsf_th *t) {	
	free(t->name);
	LIST_REMOVE_ELEM(&lwsf_world.world, t);
	free(t); 	
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
	  //LIST_PRINT(&lwsf_world.blocked);
	  printd("removed\n");
	  LIST_REMOVE_ELEM(&lwsf_world.blocked, dst); 
	  //LIST_PRINT(&lwsf_world.blocked);
	  LIST_INSERT_TAIL(&lwsf_world.ready, dst);
	  
	  // send is non blocking, no need to call schedule SCHEDULE();

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
    //LIST_PRINT(&lwsf_world.blocked);
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
  struct lwsf_th *th;
  void *never_used; 
  if(handler0)
    handler0();

  lwsf_world.thread_mc = lwsf_mem_cache_create(sizeof(struct lwsf_th));

  th = lwsf_thread_new("idle", NULL, NULL);
  idle_thread = th;
  idle_thread->entry = (void*)handler1; 
  current = th;
  /* Remove idle from blocked threads */
  LIST_REMOVE_HEAD(&lwsf_world.blocked);

  printd("swapping contexts\n");
  lwsf_arch_thread_swap(&never_used, idle_thread->context);

  //never reached  
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
