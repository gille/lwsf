#include "list.h"
#include "lwsf_mem_cache.h"

#include <stdlib.h>

struct lwsf_mem_cache {
  struct lwsf_list free; 
  struct lwsf_list mem;
  int block_size; 

  int blocks; 
  int freed;
};

#define BLOCKS 1000

static void lwsf_mem_cache_grow(struct lwsf_mem_cache *c) {
  unsigned char *p; 
  int i;
  struct lwsf_list_elem *l;

  l = (struct lwsf_list_elem*)malloc(c->block_size*BLOCKS+sizeof(struct lwsf_list_elem)); 
  l++;
  LIST_INSERT_TAIL(&c->mem, l);
  p = (unsigned char*)l;
  for(i=0; i < BLOCKS; i++) {
    LIST_INSERT_TAIL(&c->free, p);
    p += c->block_size;
  }
  c->freed += BLOCKS;
  c->blocks += BLOCKS;

}

/* FIXME: Add name! */
#define ARCH_CACHE_LINE (128)
#define CACHE_LINE_ALIGN(x) ((((x+ARCH_CACHE_LINE-1)/ARCH_CACHE_LINE)*ARCH_CACHE_LINE))
lwsf_mem_cache * lwsf_mem_cache_create(int block_size) {
  struct lwsf_mem_cache *c;
  int i;
  unsigned char *p; 

  block_size += sizeof (unsigned long);

  block_size = CACHE_LINE_ALIGN(block_size);

  c= malloc(block_size*BLOCKS + sizeof(struct lwsf_mem_cache)); 
  if(c == NULL) {
    return NULL;
  }
  c->block_size = block_size;
  c->freed = BLOCKS;
  c->blocks = BLOCKS;
  p = (unsigned char*)(++c); 
  LIST_INIT(&c->free);
  LIST_INIT(&c->mem);
  for(i=0; i < BLOCKS; i++) {
    LIST_INSERT_TAIL(&c->free, p);
    p += block_size;
  }
  return c;
}

int lwsf_mem_cache_destroy(struct lwsf_mem_cache *c) {
  
  /* we can't destroy a cache right now... */
  if(c->freed == c->blocks) {
    struct lwsf_list_elem *le;
    /* we have all the memory, iterate through and destroy extra blocks */
    
    for(le = c->mem.head; le != NULL; le = le->next) {
      free(le);
    }
    //free(c); // This causes some major badness
    
    return 0;
  }

  return (c->blocks - c->freed);
}



void *lwsf_mem_cache_alloc(struct lwsf_mem_cache *c) {
  unsigned long *r = (unsigned long*)(&c->free);

  if(r == NULL) {  
    lwsf_mem_cache_grow(c); 
    r = (unsigned long*)(&c->free.head);    
  }

  LIST_REMOVE_HEAD(&c->free);
  c->freed--;
  r[0] = (unsigned long)c;

  return (void*)&r[1];
}

void lwsf_mem_cache_free(void *m) {
  unsigned long *p = m;
  struct lwsf_mem_cache *c;

  if(p != NULL) {
    p--;
    c=(struct lwsf_mem_cache*)(p);
    c->freed++;
    /* Insert it at the head to keep cache warmer */
    LIST_INSERT_HEAD(&c->free, p); 
  }
}

