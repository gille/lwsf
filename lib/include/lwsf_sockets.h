#include <sys/socket.h>
#ifndef LWSF_SOCKETS_H
#define LWSF_SOCKETS_H

struct sockaddr;

int lwsf_accept(int fd, struct sockaddr *sockaddr, size_t *size);
void lwsf_close(int fd);
size_t lwsf_read(int fd, char *buf, size_t size);
size_t lwsf_write(int fd, char *buf, size_t size);
int lwsf_socket(int family, int type, int proto);
int lwsf_bind(int socket, struct sockaddr *saddr, size_t saddr_size);

#endif
