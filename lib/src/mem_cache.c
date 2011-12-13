#include "list.h"

struct cache {
  struct list free; 
  int block_size; 
};


static void m_cache_grow(struct cache *c) {
  unsigned char *p; 
  p = malloc(c->block_size*BLOCKS); 
  for(i=0; i < blocks; i++) {
    LIST_INSERT_TAIL(l, p);
    p += block_size;
  }
}

#define BLOCKS 1000
void * m_cache_create(int block_size) {
  struct cache *c;
  int i;
  unsigned char *p; 

  block_size += sizeof (unsigned long);
  c= malloc(block_size*BLOCKS + struct cache); 
  if(c == NULL) {
    return NULL;
  }
  c->block_size = block_size;
  p = ++c; 
  LIST_INIT(&c->free);
  for(i=0; i < blocks; i++) {
    LIST_INSERT_TAIL(l, p);
    p += block_size;
  }
}

void m_cache_destroy() {
}



void *m_cache_alloc(struct cache *c) {
  unsigned long *r = (unsigned long*)LIST_GET_HEAD(&c->free);

  if(r == NULL) {  
    m_cache_grow(c); 
    r = (unsigned long*)LIST_GET_HEAD(&c->free);
  }

  LIST_POP_HEAD(&c->free);

  r[0] = c;

  return (void*)&r[1];
}

void m_cache_free(void *m) {
  unsigned long *p = m;
  struct cache *c;
  if(p != NULL) {
    c=(p-1);
  }
  LIST_ADD_TAIL(&c->free, p); 
}

