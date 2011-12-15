#ifndef LWSF_INTERNAL_H
#define LWSF_INTERNAL_H

extern void * lwsf_arch_create_context(int size);
extern void lwsf_arch_thread_swap(void *out, void *in); 
extern void lwsf_thread_entry(void);

#endif
