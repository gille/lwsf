#ifndef LWSF_SOCKETS_H
#define LWSF_SOCKETS_H

int lwsf_accept(int fd, struct sockaddr *sockaddr, size_t *size);
void lwsf_close(int fd);
size_t lwsf_read(int fd, char *buf, size_t size);
size_t lwsf_write(int fd, char *buf, size_t size);
#endif
