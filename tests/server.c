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

#define TABLE_SIZE      100
#define MAX_DOMAIN_NAME 100

struct net_link {
    int sockfd;
};

int main(int argc, char** argv) {

    int i, j, k;    // used for all forloops
    int sockfd;     // Nonblocking listening port
    int newfd;      // Used for getting new connection fds from sockfd
    char s0[INET6_ADDRSTRLEN]; // the address listening to

    char domain[MAX_DOMAIN_NAME]; // domain read from argv
    int tcp;                      // The port number to listen

    struct addrinfo  hints;    // used to select port config options
    struct addrinfo* servinfo; // pointer to results of setup


    // Ensure user has a entered possible address and port
    if(argc != 3) {
        printf("Need to execute with local listen address and port\n");
        exit(1);
    }

    // read address
    sscanf(argv[1], "%s", domain);
    // read port
    sscanf(argv[2], "%d", &tcp);
    // print to check
    printf("Address     : %s\n", domain);
    printf("Port        : %d\n", tcp);
    printf("argv Address: %s\n", argv[1]);
    printf("argv Port   : %s\n", argv[2]);


    // Make sure hints is empty because it uses bits
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;   // dont care ipv4 / ipv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    // getaddrinfo populates servinfo with information used to make
    // a socket. It returns 0 if success.
    if((getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        printf("getaddrinfo failed!\n");
        exit(1);
    }

    // Print information to make sure everything is ready
    struct addrinfo* p;
    for(p = servinfo; p != NULL; p = p->ai_next) {
        void* addr;
        char* ipver;
        int yes = 1;

        // get pointer to address itself
        if(p->ai_family == AF_INET) { // ipv4
            struct sockaddr_in* ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } 
        else { // IPv6
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        // convert IP to string and print
        inet_ntop(p->ai_family, addr, domain, sizeof domain);
        printf("%s        : %s\n", ipver, domain);
    }

    // Make the actual socket using info in servinfo
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    printf("SockFD: %d\n", sockfd);

    // Set socket options, allow other sockets to bind to port
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    // Set socket to non blocking
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    // Need to bind it to a port so we can listen, port is that passed to getaddrinfo
    bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

    // TODO: LISTEN TO SOCKET

    // Once used, delete the linked list
    freeaddrinfo(servinfo);


    // Create and initialize array of all net_links
    struct net_link** all_links =
        (struct net_link**) malloc(TABLE_SIZE * sizeof(struct net_link*));
    for(i = 0; i < TABLE_SIZE; ++i) all_links[i] = NULL;

    // create and initialize array of valid links sorted by host id
    struct net_link** table = 
        (struct net_link**) malloc(TABLE_SIZE * sizeof(struct net_link*));
    for(i = 0; i < TABLE_SIZE; ++i) table[i] = NULL;

    srand(time(NULL));

    while(1) {
        return 0;
    }

    return 0;
}
