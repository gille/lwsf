#include "lwsf.h"
#include "list.h"
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include "lwsf_sockets.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

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
static void * accept_server(void *arg) {
     fd_set r, _r; 
     int max=0; 
     struct msg *m;
     char buf[128];
     struct lwsf_list l;
     struct lwsf_list_elem *e;

     LIST_INIT(&l);

     FD_ZERO(&r);
     FD_ZERO(&_r);
     FD_SET(accept_pipe[0], &r);
     max = accept_pipe[0];
     for(;;) {
	  _r = r;
	  if (select(max+1, &_r, 0, 0, 0) > 0) {
	       /**/      
	       printf("got out of select!\n");
	       if(FD_ISSET(accept_pipe[0], &_r)) {
		    /* Ok this was the easy one */
		    printf("We have something in the read pipe!\n");
		    read(accept_pipe[0], buf, sizeof(buf)); 
		    do { 
			 m = (struct msg*)lwsf_msg_recv_try(acceptq); 
			 if(m != NULL) {
			      FD_SET(m->fd, &r);
			      printf("Ok wait for FD: %d\n", m->fd);
			      max = MAX(max, m->fd); 
			      e = malloc(sizeof(*e));
			      e->data=m;
			      LIST_INSERT_TAIL(&l, e); 
			 }
		    } while(m != NULL);		    
	       } else {
		    /* Find the originating file descriptor */
		    for(e=l.head; e != NULL; e = e->next) {
			 if(e->data != NULL) {
			      m = e->data;
			      if(FD_ISSET(m->fd, &r)) {
				   printf("Found a set descriptor! %d\n", m->fd);
				   FD_CLR(m->fd, &r); 
				   lwsf_msg_send((void**)&m, lwsf_msg_sender(m)); 
			      }
			 }
		    }
	       }
	  }
     }
     return NULL;
}

static void * read_server(void *arg) {
     fd_set r, _r; 
     int max=0; 
     struct msg *m;
     char buf[128];
     struct lwsf_list l;
     struct lwsf_list_elem *e;

     LIST_INIT(&l);
     FD_ZERO(&r);
     FD_ZERO(&_r);
     FD_SET(read_pipe[0], &r);
     max = read_pipe[0];

     for(;;) {
	  _r = r;
	  if (select(max+1, &_r, 0, 0, 0) > 0) {
	       /**/      
	       printf("got out of select!\n");
	       if(FD_ISSET(read_pipe[0], &_r)) {
		    /* Ok this was the easy one */
		    printf("We have something in the read pipe!\n");
		    read(read_pipe[0], buf, sizeof(buf)); 
		    do { 
			 m = (struct msg*)lwsf_msg_recv_try(readq); 
			 if(m != NULL) {
			      FD_SET(m->fd, &r);
			      printf("Ok wait for FD: %d\n", m->fd);
			      max = MAX(max, m->fd); 
			      e = malloc(sizeof(*e));
			      e->data=m;
			      LIST_INSERT_TAIL(&l, e); 
			 }
		    } while(m != NULL);		    
	       } else {
		    /* Find the originating file descriptor */
		    for(e=l.head; e != NULL; e = e->next) {
			 if(e->data != NULL) {
			      m = e->data;
			      if(FD_ISSET(m->fd, &r)) {
				   printf("Found a set descriptor! %d\n", m->fd);
				   FD_CLR(m->fd, &r); 
				   lwsf_msg_send((void**)&m, lwsf_msg_sender(m)); 
			      }
			 }
		    }
	       }
	  }
     }
     return NULL;
}

int lwsf_accept(int fd, struct sockaddr *sockaddr, size_t *size) {
     struct msg *m;
     /* We do the select in another thread context so we can KEEP running! */
     m =(struct msg*) lwsf_msg_alloc(10, 10); 
     m->fd = fd; 
     printf("got here!\n");
     printf("acceptq: %p\n", acceptq);
     lwsf_msg_sendq((void**)&m, acceptq);
     printf("try to write!\n");
     if(write(accept_pipe[1], "1", 1) != 1)
	  printf("write to pipe failed!\n");
     printf("Wrote! now wait\n");
     
     m = (struct msg*)lwsf_msg_recv(NULL); 
     return accept(fd, sockaddr, size); 
}

void lwsf_close(int fd) {
     /* send message */
     close(fd);
}

size_t lwsf_read(int fd, char *buf, size_t size) {
     struct msg *m;
     m =(struct msg*) lwsf_msg_alloc(10, 10); 
     m->fd = fd;
     lwsf_msg_sendq(&m, readq);
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
     printf("%p\n", acceptq); 
     pipe(accept_pipe);
     pipe(read_pipe);

     pthread_create(&read_thread, NULL, read_server, NULL); 
     pthread_create(&accept_thread, NULL, accept_server, NULL); 
}

int lwsf_socket(int family, int type, int proto) {
    return socket(family, type, proto); 
}

int lwsf_bind(int socket, struct sockaddr *saddr, size_t saddr_size) {
    return bind(socket, saddr, saddr_size); 
}
