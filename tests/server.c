/* File: server.c
 * Name: Paulo Lemus
 * Date: 4/6/2017
 */

/* This program contains code for a "switch"
 * It has has a listening socket, as well as a
 * list of fd/host_ids.
 *
 * It accepts listens on non-blocking port for new
 * connections.
 * Otherwise if it has any connections it randomly
 * sends a packet to a host.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>

struct net_link {
    int sockfd;
}

int main(int argc, char** argv) {

    int sockfd; // Nonblocking listening port
    int newfd;  // Used for getting new connection fds from sockfd



    struct net_link**
    srand(time(NULL));

    int 
    while(1) {
        
    }

    return 0;
}
