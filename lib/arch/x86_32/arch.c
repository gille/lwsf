#include <stdlib.h>

#include "lwsf.h"
#include "lwsf_internal.h"

/* FIXME: Everybody has a private stack? YES! Otherwise we can only do run-to-completion */
void * lwsf_arch_create_context(int size) {
    /* FIXME: Make only one malloc */
    //unsigned long *t = malloc(7*4); 
    unsigned char *s = malloc(size+7*4);
    unsigned long *t = s; 
    unsigned long *x;
    s += (7*4+sizeof(unsigned long)-1)/sizeof(unsigned long);
    
    
    s += size - 4;
    
    x = (unsigned long*)(s);
    *(x) = (unsigned long)lwsf_thread_entry; /* return address */
    x--;
    *x = 0xBE; /* AX */ /* AX */
    //  x--;
    /* EBX, EDI, ESI, ESP, EBP */
    t[0] = 0; /* BX */
    t[1] = 0; /* CX */
    t[2] = 0; /* DX */
    t[3] = 0; /* SI */
    t[4] = 0; /* DI */
    t[5] = (unsigned long)x; /* SP */
    t[6] = (unsigned long)(s+4); /* BP */
    
    return t;    
}
