#ifndef LWSF_MEM_CACHE_H
#define LWSF_MEM_CACHE_H

struct lwsf_mem_cache;
typedef struct lwsf_mem_cache lwsf_mem_cache;

extern lwsf_mem_cache * lwsf_mem_cache_create(int block_size);
extern int lwsf_mem_cache_destroy(lwsf_mem_cache *c);

extern void *lwsf_mem_cache_alloc(struct lwsf_mem_cache *c);
extern void lwsf_mem_cache_free(void *m);

#endif
