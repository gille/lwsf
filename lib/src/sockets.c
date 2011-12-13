#include "lwsf.h"
#include "list.h"
#include <sys/socket.h>
#include <stdint.h>

struct {
  uint32_t mid;
  int fd;
};

static void * accept_server(void *arg) {
  fd_set r; 
  int max=0; 
  FD_ZERO(&r);
  
  for(;;) {
    if (accept(max+1, &r, 0, 0, 0) > 0) {
      /**/      
      
    }
    m = lwsf_receive_msg(NULL); 
    if(m != NULL) {
      FD_SET(r, m->fd);
    }
  }
}

int lwsf_accept(int fd, struct sockaddr *sockaddr, size_t *size) {
  m = lwsf_receive_msg(NULL); 
  return accept(fd, sockaddr, size); 
}

void lwsf_close(int fd) {
  /* send message */
  close(fd);
}

size_t lwsf_read(int fd, char *buf, size_t size) {
}

size_t lwsf_write(int fd, char *buf, size_t size) {
  /* FIXME? */
  return write(fd, buf, size); 
}

void init_socket_servers() {
}
