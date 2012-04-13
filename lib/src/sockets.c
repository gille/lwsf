#include <assert.h>
#include "lwsf.h"
#include "list.h"
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include "lwsf_sockets.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <strings.h>

struct msg {
     uint32_t mid;
     int fd;
};

static lwsf_msg_queue *acceptq;
static lwsf_msg_queue *readq;
int accept_pipe[2];
int read_pipe[2];
//static struct lwsf_msg_queue *write_queue;

/* FIXME: We must use a pipe or something with a file descriptor to send messages to the 
 * accept/read server. Otherwise things will get lost/blocked. 
 */
#define MAX(a,b) ((a)>(b)?(a):(b))
/* OK on Linux this applies */
static struct msg *accept_msgs[1024];
static struct msg *read_msgs[1024];

static void * accept_server(void *arg) {
     fd_set r, _r; 
     int max=0; 
     struct msg *m;
     char buf[128];
     int i;
     long *bits; 
     int n;
     int fd;

     FD_ZERO(&r);
     FD_ZERO(&_r);
     FD_SET(accept_pipe[0], &r);
     max = accept_pipe[0];
     bits = __FDS_BITS(&_r); 

     for(;;) {
	  _r = r;
	  n = select(max+1, &_r, 0, 0, 0);
	  if(n > 0) {
	       if(FD_ISSET(accept_pipe[0], &_r)) {
		    /* Ok this was the easy one */
		    if(read(accept_pipe[0], buf, sizeof(buf)) < 0)
			 abort();
		    do { 
			 m = (struct msg*)lwsf_msg_recv_try(acceptq); 
			 if(m != NULL) {
			      FD_SET(m->fd, &r);
			      max = MAX(max, m->fd); 
			      assert(accept_msgs[m->fd] == NULL);
			      accept_msgs[m->fd]=m;
			 }
		    } while(m != NULL);		    
		    n--;
	       }
	       if(n > 0) {
		 for(i = 0; (i*32) < max && i < __FD_SETSIZE/ __NFDBITS; i++) {
			 while(bits[i] != 0) {
			      fd = ffs(bits[i])+32*i-1; /* ? */
			      bits[i] &= ~(1<<fd); 
			      m = accept_msgs[fd];
			      assert(m != NULL);
			      accept_msgs[fd] = NULL;
			      FD_CLR(m->fd, &r); 
			      lwsf_msg_send((void**)&m, lwsf_msg_sender(m)); 
			      if(--n == 0) 
				   break;
			 }
		    }
	       }
	       assert(n==0);
	  }
     }
     return NULL;
}

static void * read_server(void *arg) {
     fd_set r, _r; 
     int max=0; 
     struct msg *m;
     char buf[128];
     long *bits;
     int i;
     int n;

     bits = __FDS_BITS(&_r); 
     FD_ZERO(&r);
     FD_ZERO(&_r);
     FD_SET(read_pipe[0], &r);
     max = read_pipe[0];

     for(;;) {
	  _r = r;
	  n = select(max+1, &_r, 0, 0, 0);
	  if(n > 0) {
	       /**/      
	       if(FD_ISSET(read_pipe[0], &_r)) {
		    /* Ok this was the easy one */
		    if(read(read_pipe[0], buf, sizeof(buf)) < 0)
			 abort();
		    do { 
			 m = (struct msg*)lwsf_msg_recv_try(readq); 
			 if(m != NULL) {
			      FD_SET(m->fd, &r);
			      max = MAX(max, m->fd); 
			      assert(m->fd > 0 && m->fd < 1024);
			      assert(read_msgs[m->fd] == NULL); 
			      read_msgs[m->fd] = m;
			 }
		    } while(m != NULL);
		    n--;
	       }
	       if(n > 0) {
		 for(i = 0; (i*32) < max && i < __FD_SETSIZE/ __NFDBITS; i++) {
			 while(bits[i] != 0) {
			      int fd;
			      fd = ffs(bits[i])+32*i-1; /* ? */
			      bits[i] &= ~(1<<fd); 
			      m = read_msgs[fd];
			      assert(m != NULL);
			      read_msgs[fd] = NULL;
			      FD_CLR(m->fd, &r); 
			      lwsf_msg_send((void**)&m, lwsf_msg_sender(m)); 
			      if(--n == 0) 
				   break;
			 }
		    }
	       }
	       assert(n==0);
	  }
     }
     return NULL;
}

int lwsf_accept(int fd, struct sockaddr *sockaddr, size_t *size) {
     struct msg *m;
     /* We do the select in another thread context so we can KEEP running! */
     m =(struct msg*) lwsf_msg_alloc(10, 10); 
     m->fd = fd; 
     lwsf_msg_sendq((void**)&m, acceptq);
     if(write(accept_pipe[1], "1", 1) != 1)
	  printf("write to pipe failed!\n");
     
     m = (struct msg*)lwsf_msg_recv(NULL); 
     return accept(fd, sockaddr, size); 
}

void lwsf_close(int fd) {
     /* send message */
     close(fd);
}

size_t lwsf_read(int fd, char *buf, size_t size) {
     struct msg *m;
     m = (struct msg*) lwsf_msg_alloc(10, 10); 
     m->fd = fd;
     lwsf_msg_sendq((void**)&m, readq);
     if(write(read_pipe[1], "1", 1) != 1)
     m = lwsf_msg_recv(NULL);
     return read(fd, buf, size); 
}

size_t lwsf_write(int fd, char *buf, size_t size) {
     /* FIXME? */
     return write(fd, buf, size); 
}

void lwsf_init_socket_servers() {
     pthread_t read_thread;
     pthread_t accept_thread;
  
     readq = lwsf_msgq_create();
     acceptq = lwsf_msgq_create();
     if(pipe(accept_pipe) == -1) {
	  printf("ERROR: Unable to create pipe!\n");
	  exit(-1);
     }
     if(pipe(read_pipe) == -1) {
	  printf("ERROR: Unable to create pipe!\n");
	  exit(-1);
     }

     pthread_create(&read_thread, NULL, read_server, NULL); 
     pthread_create(&accept_thread, NULL, accept_server, NULL); 
}

int lwsf_socket(int family, int type, int proto) {
    return socket(family, type, proto); 
}

int lwsf_bind(int socket, struct sockaddr *saddr, size_t saddr_size) {
    return bind(socket, saddr, saddr_size); 
}
