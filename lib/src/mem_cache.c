#include "list.h"
#include "lwsf_mem_cache.h"

#include <stdlib.h>

struct lwsf_mem_cache {
  struct lwsf_list free; 
  int block_size; 
};

#define BLOCKS 1000

static void lwsf_mem_cache_grow(struct lwsf_mem_cache *c) {
  unsigned char *p; 
  int i;

  p = malloc(c->block_size*BLOCKS); 
  for(i=0; i < BLOCKS; i++) {
    LIST_INSERT_TAIL(&c->free, p);
    p += c->block_size;
  }
}


lwsf_mem_cache * lwsf_mem_cache_create(int block_size) {
  struct lwsf_mem_cache *c;
  int i;
  unsigned char *p; 

  block_size += sizeof (unsigned long);
  c= malloc(block_size*BLOCKS + sizeof(struct lwsf_mem_cache)); 
  if(c == NULL) {
    return NULL;
  }
  c->block_size = block_size;
  p = (unsigned char*)(++c); 
  LIST_INIT(&c->free);
  for(i=0; i < BLOCKS; i++) {
    LIST_INSERT_TAIL(&c->free, p);
    p += block_size;
  }
  return c;
}

void lwsf_mem_cache_destroy(struct lwsf_mem_cache *c) {
  /* we can't destroy a cache right now... */
}



void *lwsf_mem_cache_alloc(struct lwsf_mem_cache *c) {
  unsigned long *r = (unsigned long*)(&c->free);

  if(r == NULL) {  
    lwsf_mem_cache_grow(c); 
    r = (unsigned long*)(&c->free.head);    
  }

  LIST_REMOVE_HEAD(&c->free);

  r[0] = (unsigned long)c;

  return (void*)&r[1];
}

void lwsf_mem_cache_free(void *m) {
  unsigned long *p = m;
  struct lwsf_mem_cache *c;
  if(p != NULL) {
    p--;
    c=(struct lwsf_mem_cache*)(p);
  
    LIST_INSERT_TAIL(&c->free, p); 
  }
}

