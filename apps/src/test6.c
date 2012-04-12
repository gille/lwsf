#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "lwsf.h"
#include "lwsf_mem_cache.h"
#include "lwsf_sockets.h"

#include <netinet/in.h>

#if 0
#define printd(fmt, args...)
#else
#define printd(fmt, args...) { printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args); }
#endif

void ping(void *arg) {
    struct sockaddr_in saddr;
    size_t slen;
    int server = lwsf_socket(AF_INET, SOCK_STREAM, 0); 
    int on = 1;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(12345);
    
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    lwsf_bind(server, (struct sockaddr*)&saddr, sizeof(saddr)); 
    slen = sizeof(saddr);
    listen(server, 12);
    printf("My socket: %d\n", server); 
    lwsf_accept(server, (struct sockaddr*)&saddr, &slen); 
    printf("So frickin done!\n");
}

void handler1(void) {
    lwsf_thread_new("p1", ping, NULL);
}

int main(void) {
    lwsf_start(NULL, handler1);
    return 0;
}
