#include "lwsf.h"
#include "list.h"
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include "lwsf_sockets.h"

struct msg {
  uint32_t mid;
  int fd;
};

static struct msg_queue accept_queue;
static struct msg_queue read_queue;
static struct msg_queue write_queue;

static void * accept_server(void *arg) {
  fd_set r; 
  int max=0; 
  struct msg *m;
  FD_ZERO(&r);
  
  for(;;) {
    if (select(max+1, &r, 0, 0, 0) > 0) {
      /**/      
      
    }
    m = lwsf_receive_msg(NULL); 
    if(m != NULL) {
      FD_SET(m->fd, &r);
    }
  }
}

static void * read_server(void *arg) {
  fd_set r; 
  int max=0; 
  struct msg *m;

  FD_ZERO(&r);
  
  for(;;) {
    if(FD_ISSET(0, &r)) {
      if (read(max+1, &r, 0, 0, 0) > 0) {
	/**/
      }
    }
    m = lwsf_receive_msg(NULL); 
    if(m != NULL) {
      FD_SET(m->fd, &r);
    }
  }
}

int lwsf_accept(int fd, struct sockaddr *sockaddr, size_t *size) {
  struct msg *m;
  m = lwsf_receive_msg(NULL); 
  return accept(fd, sockaddr, size); 
}

void lwsf_close(int fd) {
  /* send message */
  close(fd);
}

size_t lwsf_read(int fd, char *buf, size_t size) {
  lwsf_send();
  lwsf_receive();
  return read(fd, buf, size); 
}

size_t lwsf_write(int fd, char *buf, size_t size) {
  /* FIXME? */
  return write(fd, buf, size); 
}

void init_socket_servers() {
  pthread_t read_thread;
  pthread_t accept_thread;
  
  msg_queue m = lwsf_msgq_create();

  pthread_create(&read_thread, NULL, read_server, NULL); 
  pthread_create(&accept_thread, NULL, accept_server, NULL); 
}
