#include <stdlib.h>

#include "lwsf.h"

extern void thread_entry();
void * create_context(int size) {
  unsigned int *t = malloc(7*4); 
  unsigned char *s = malloc(size);
  unsigned int *x;


  s += size - 4;
  
  x = (unsigned int*)(s);
  *(x) = thread_entry; /* return address */
  x--;
  *x = 0xBE; /* AX */ /* AX */
  //  x--;
  /* EBX, EDI, ESI, ESP, EBP */
  t[0] = 0; /* BX */
  t[1] = 0; /* CX */
  t[2] = 0; /* DX */
  t[3] = 0; /* SI */
  t[4] = 0; /* DI */
  t[5] = x; /* SP */
  t[6] = s+4; /* BP */

  return t;    
}
