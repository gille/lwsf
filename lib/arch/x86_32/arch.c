#include <stdlib.h>

#include "lwsf.h"
#include "lwsf_internal.h"

void * lwsf_arch_create_context(int size) {
  unsigned long *t = malloc(7*4); 
  unsigned char *s = malloc(size);
  unsigned long *x;


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
