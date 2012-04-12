#include "lwsf.h"
#include "list.h"
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include "lwsf_sockets.h"
#include <unistd.h>
#include <pthread.h>

struct msg {
     uint32_t mid;
     int fd;
};

static lwsf_msg_queue *acceptq;
static lwsf_msg_queue *readq;
//static struct lwsf_msg_queue *write_queue;

static void * accept_server(void *arg) {
     fd_set r; 
     int max=0; 
     struct msg *m;
     FD_ZERO(&r);
  
     for(;;) {
	  if (select(max+1, &r, 0, 0, 0) > 0) {
	       /**/      
      
	  }
	  m = (struct msg*)lwsf_msg_recv(acceptq); 
	  if(m != NULL) {
	       FD_SET(m->fd, &r);
	  }
     }
     return NULL;
}

static void * read_server(void *arg) {
     fd_set r; 
     int max=0; 
     struct msg *m;

     FD_ZERO(&r);
  
     for(;;) {
	  if(FD_ISSET(0, &r)) {
	       if (select(max+1, &r, 0, 0, 0) > 0) {
		    /**/
	       }
	  }
	  m = (struct msg*)lwsf_msg_recv(NULL); 
	  if(m != NULL) {
	       FD_SET(m->fd, &r);
	  }
     }
     return NULL;
}

int lwsf_accept(int fd, struct sockaddr *sockaddr, size_t *size) {
     struct msg *m;
     m =(struct msg*) lwsf_msg_alloc(10, 10); 
     //  lwsf_msg_sendq();
     m = (struct msg*)lwsf_msg_recv(NULL); 
     return accept(fd, sockaddr, size); 
}

void lwsf_close(int fd) {
     /* send message */
     close(fd);
}

size_t lwsf_read(int fd, char *buf, size_t size) {
     void *m = NULL;
     lwsf_msg_sendq(&m, readq);
     m = lwsf_msg_recv(NULL);
     return read(fd, buf, size); 
}

size_t lwsf_write(int fd, char *buf, size_t size) {
     /* FIXME? */
     return write(fd, buf, size); 
}

void init_socket_servers() {
     pthread_t read_thread;
     pthread_t accept_thread;
  
     readq = lwsf_msgq_create();
     acceptq = lwsf_msgq_create();

     pthread_create(&read_thread, NULL, read_server, NULL); 
     pthread_create(&accept_thread, NULL, accept_server, NULL); 
}
