#include <assert.h>

#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "lwsf.h"
#include "lwsf_mem_cache.h"
#include "lwsf_internal.h"
#include "arch_internal.h"

#define THREAD_TYPE_LWSF   0
#define THREAD_TYPE_SYSTEM 1


#define STATE_RUNNING             0
#define STATE_READY               1

#define STATE_BLOCKED_MESSAGE     4
#define STATE_BLOCKED             8

#define STATE_STOPPED             16

#define KILL_THREAD 0x1000

struct msg {
     uint32_t id;
     struct lwsf_th *th;
};

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

     struct lwsf_mem_cache *thread_mc;

     struct lwsf_th *idle_thread;
} lwsf_world;

struct lwsf_msg {
     struct lwsf_list_elem n;
     struct lwsf_th *sender;
     unsigned long aligner;/* fixme */
     unsigned char data[1];
};

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
	  if(!((is_in(&lwsf_world.blocked, n->data)) ||is_in(&lwsf_world.ready, n->data) || (n->data+12) != lwsf_world.idle_thread)) {
	       printf("A threadlet has been lost! %p\n", n->data);
	       printf("%p\n", lwsf_world.idle_thread);
	       printf("A threadlet has been lost! %#x\n", ((unsigned long)n)-12);
	       exit(-1);
	  }
}
#endif

static void lwsf_thread_delete(struct lwsf_th *t) {	
     assert(t != lwsf_world.current);
     LIST_REMOVE_ELEM(&lwsf_world.world, &t->n2);
     if(t->name)
	  free(t->name);
     if(t->stack)
	  free(t->stack);

     lwsf_mem_cache_free(t);	
}

static void SCHEDULE(void) {
     struct lwsf_th *old, *new;
     printd("schedule called\n");
#ifdef ASSERT_BUILD
     ASSERT_WORLD();
#endif
     if(lwsf_world.ready.head == NULL) {
	  printd("ready_head is NULL swap to idle\n");
	  old = lwsf_world.current; 
	  new = lwsf_world.idle_thread; 
	  lwsf_world.current = new; 
	  lwsf_arch_thread_swap(old->context, new->context); 
     } else {
	  printd("ready_head is: %p %p\n", lwsf_world.ready.head, lwsf_world.current);
	  if(lwsf_world.ready.head != (void*)lwsf_world.current) {
	       printd("swap to %s\n", ((struct lwsf_th*)LIST_GET_HEAD(&lwsf_world.ready))->name);
	       old = lwsf_world.current; 
	       new = ((struct lwsf_th*)LIST_GET_HEAD(&lwsf_world.ready)); 
	       lwsf_world.current = new; 
	       printd("swapping %p %p \n", old, new);
	       lwsf_arch_thread_swap(old->context, new->context); 
	  }
     }
}

/* FIXME: Make this static, needs new parameter for create context */
void lwsf_thread_entry(void) {
     struct lwsf_th *th;
     void (*handler1)(void);
     printd("here\n");
     if((lwsf_world.current == lwsf_world.idle_thread)) {
	  lwsf_init_socket_servers();
	  handler1 = (void*)lwsf_world.current->entry; 
	  //printd("calling hook1\n");
	  handler1();
    
	  while((th = LIST_GET_HEAD(&lwsf_world.blocked)) != NULL) {
	       LIST_REMOVE_HEAD(&lwsf_world.blocked);
	       th->state = STATE_READY;
	       LIST_INSERT_TAIL(&lwsf_world.ready, th);
	       //printd("Thread %s made ready\n", th->name);
	  }
	  SCHEDULE();
	  /* this is now the idle loop */
	  for(;;) {
	       struct msg *m;
	       if(lwsf_world.world.head == lwsf_world.world.tail) {
		   printf("IN IDLE and only guy, lets leave\n");
		   exit(0);
		   /* There are no other threads.. */
		   break;
	       }
	       m = lwsf_msg_recv_try(NULL); 
	       if(m != NULL) {
		    printf("%x\n", m->id);
		    switch(m->id) {
			
		    case KILL_THREAD:
			 lwsf_thread_delete(m->th);
			 break;
		    default:
			 printf("Unknown message!\n");
			 exit(-1);
			 break;
		    }
		    lwsf_msg_free((void**)&m);
	       }
	       SCHEDULE();
	  }
     } 
     printd("calling entry point\n");
     lwsf_world.current->entry(lwsf_world.current->arg);
     lwsf_thread_kill(lwsf_world.current);
     /* never reached */
}

static struct lwsf_th *lwsf_thread_internal_new(const char *name, int type) {
     struct lwsf_th *t; 
     struct lwsf_list_elem *l;

     t = lwsf_mem_cache_alloc(lwsf_world.thread_mc);
     if(t == NULL) 
	  goto out;

     t->name = strdup(name);
     if(t->name == NULL) 
	  goto out_name;
     l = (struct lwsf_list_elem*)t;
     l->data = t; 
     (l+1)->data = t; 

     LIST_INSERT_TAIL(&lwsf_world.world, (l+1));
     LIST_INIT(&t->mailbox.messages);
     LIST_INIT(&t->mailbox.blocked);

     t->type = type;

     return t;
out_name:
     free(t);
out:
     return NULL;
}

struct lwsf_th *lwsf_thread_sys_new(const char *name) {
     struct lwsf_th *t = lwsf_thread_internal_new(name, THREAD_TYPE_SYSTEM); 
     if(t == NULL) 
	  return NULL;
    
     t->stack = NULL; 
     t->arg = NULL;
     t->entry = NULL;
     t->state = STATE_RUNNING;
     return t;
}

struct lwsf_th* lwsf_thread_new(const char *name, void (*entry)(void*), void *arg) {
     struct lwsf_th *t; 
     int stack_size = 1000*1000;
     void *stack;

     t = lwsf_thread_internal_new(name, THREAD_TYPE_LWSF);
     if(t == NULL)
	  goto out; 
     /* FIXME: Convert this to a mem cache object? */
     stack = malloc(stack_size);
     if(stack == NULL) 
	  goto out_stack;
     LIST_INSERT_TAIL(&lwsf_world.blocked, t);

     t->entry = entry;
     t->arg = arg; 
     lwsf_arch_create_context(t->context, stack, stack_size);
     t->stack = stack;
     t->state = STATE_BLOCKED;
     printd("%s [%p] context created at %p\n", t->name, t, t->context);
     return t;

out_stack:
     free(t);
out:
     return NULL;
}

void lwsf_thread_kill(struct lwsf_th *t) {
     if(t == lwsf_world.current) {
	  LIST_REMOVE_ELEM(&lwsf_world.ready, t);
	  struct msg *msg = lwsf_msg_alloc(sizeof(struct msg), KILL_THREAD); 
	  msg->th=t;
	  lwsf_msg_send((void**)&msg, lwsf_world.idle_thread); 
	  printf("Oooh schedule!\n");
	  SCHEDULE(); 	  
	  /* THIS IS NEVER REACHED! */
     } else {
	  /* FIXME: Let us kill other threads? */
	  exit(-1);
	  lwsf_thread_delete(t);
     }    


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
     m->sender = lwsf_world.current; 
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
     m->sender = lwsf_world.current; 

     LIST_INSERT_TAIL(&(dst->mailbox.messages), m);	

     if(dst->state == STATE_BLOCKED_MESSAGE) {
	  printd("blocked!\n");
	  //LIST_PRINT(&lwsf_world.blocked);
	  printd("removed\n");
	  LIST_REMOVE_ELEM(&lwsf_world.blocked, dst); 
	  assert(dst->state <= STATE_BLOCKED);
	  dst->state = STATE_READY;
	  //LIST_PRINT(&lwsf_world.blocked);
	  LIST_INSERT_TAIL(&lwsf_world.ready, dst);
	  /* Yes! We need to call schedule */
	  SCHEDULE();
     }
}

void *lwsf_msg_recv_try(lwsf_msg_queue *m) {
     struct lwsf_msg *msg;

     if(m == NULL) {
	  m = &lwsf_world.current->mailbox;
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
	  if(lwsf_world.current->state == STATE_READY) {
	       LIST_REMOVE_HEAD(&lwsf_world.ready);
	       LIST_INSERT_TAIL(&lwsf_world.blocked, lwsf_world.current); 
	       //LIST_PRINT(&lwsf_world.blocked);
	       lwsf_world.current->state = STATE_BLOCKED_MESSAGE;
	       
	       if(m != NULL) {
		    LIST_INSERT_TAIL(&(m->blocked), lwsf_world.current);
	       }
	       SCHEDULE();
	  }
     } 
  
     return (msg); 
}

struct lwsf_th* lwsf_msg_sender(void *m) { 
     struct lwsf_msg *msg = m;
     msg--;
     return msg->sender;
}

void lwsf_thread_stop(struct lwsf_th *t) { 
     if(t == lwsf_world.current) {
	  printd("stopping lwsf_world.current thread\n");
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
     //  LIST_REMOVE_ELEM(&lwsf_world.ready, lwsf_world.current);
     LIST_REMOVE_HEAD(&lwsf_world.ready); 
     LIST_INSERT_TAIL(&lwsf_world.ready, lwsf_world.current);
     if(lwsf_world.current != (struct lwsf_th*)(lwsf_world.ready.head)){ 
	  SCHEDULE();
     }
}

void lwsf_start(void (*handler0)(void), void (*handler1)(void)) 
{
     int orig_state[16];

     if(handler0)
	  handler0();

     lwsf_world.thread_mc = lwsf_mem_cache_create(sizeof(struct lwsf_th));

     lwsf_world.current = lwsf_world.idle_thread = lwsf_thread_new("idle", NULL, NULL);
     lwsf_world.idle_thread->entry = (void*)handler1; 
     /* Remove idle from blocked threads */     

     LIST_REMOVE_HEAD(&lwsf_world.blocked);

     printd("swapping contexts\n");
     lwsf_arch_thread_swap(orig_state, lwsf_world.idle_thread->context);
     lwsf_thread_delete(lwsf_world.idle_thread);
     LIST_PRINT(&lwsf_world.world);
     lwsf_world.idle_thread = NULL;
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

void lwsf_msg_free(void **m) {
     struct lwsf_msg *msg;
     if(m == NULL) return;
     msg = *m;
     if(msg == NULL) return;
     msg--;     
     free(msg);
     *m = NULL; 
}

lwsf_msg_queue* lwsf_msgq_create() {
     lwsf_msg_queue *m = malloc(sizeof(lwsf_msg_queue));
  
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
