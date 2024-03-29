#ifndef LWSF_INTERNAL_H
#define LWSF_INTERNAL_H

extern void lwsf_arch_create_context(void *context, void *stack, int size); 
extern void lwsf_arch_thread_swap(void *out, void *in); 
extern void lwsf_thread_entry(void);

extern void lwsf_thread_kill(struct lwsf_th *t);
extern struct lwsf_th* lwsf_thread_sys_new(const char *name);
extern void lwsf_init_socket_servers();

#endif
