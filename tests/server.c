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
#define BACKLOG         10

struct net_link {
    int hostfd;
    struct net_link* next;
};

void remove_link(struct net_link** list, struct net_link* node) {
    if(node == NULL)        return;
    else if(*list == NULL) return;
    else {
        struct net_link* current = *list;
        while(current != NULL       && 
              current->next != NULL &&
              current->next != node) current = current->next;
        if(current == *list) {
            free(*list);
            *list = NULL;
        }
        else {
            current->next = node->next;
            free(node);
        }
    }
}

int main(int argc, char** argv) {

    int i, j, k;    // used for all forloops
    int sockfd;     // Nonblocking listening port
    int yes = 1;    // Yes
    char s0[INET6_ADDRSTRLEN]; // the address listening to

    char domain[MAX_DOMAIN_NAME]; // domain read from argv
    int tcp;                      // The port number to listen

    struct addrinfo  hints;    // used to select port config options
    struct addrinfo* servinfo; // pointer to results of setup


    // Ensure user has a entered possible address and port
    if(argc != 3) {
        printf("Need to execute with local listen address and port\n");
        printf("Example: ./a.out 127.0.0.1 3001\n");
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
    if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        printf("Failed to bind\n");
        exit(1);
    }

    // Listen on port. If listen fails, exit program
    if(listen(sockfd, BACKLOG) < 0) {
        printf("Failed to listen\n");
        exit(1);
    }

    // Once used, delete the linked list
    freeaddrinfo(servinfo);


    // Create and initialize array of all net_links
    // All new connections will be in this list
    struct net_link** all_links = 
        (struct net_link**) malloc(sizeof(struct net_link*));
    *all_links = NULL;
    struct net_link*  next_link = NULL;

    // create and initialize array of valid links sorted by host id
    // This table is indexed by host ID.
    // This saves pointers to net_link structs in the all_links list.
    struct net_link** table = 
        (struct net_link**) malloc(TABLE_SIZE * sizeof(struct net_link*));
    for(i = 0; i < TABLE_SIZE; ++i) table[i] = NULL;

    //////////////////////////////////////////////////////////////////

    srand(time(NULL));
    int newfd;                          // Used for getting new connection fds from sockfd
    struct sockaddr_storage their_addr; // Used to get info about new connections
    socklen_t addr_size;                // Get more info from new connection

    char buff[100 + 7]; // 100 chars + 3 for host, 3 for destination, 1 for '\0'
    int recv_size;


    printf("about to enter main while loop\n");
    while(1) {

        // TODO: Take care of hosts that disconnect.
        // Program currently crashes when any hosts disconnect.

        // Accept on non-blocking port
        newfd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size);
        // Add new connections to linked list of ports
        if(newfd > 0) {
            printf("Accepted new FD: %d\n", newfd);
            struct net_link* newlink = 
                (struct net_link*) malloc(sizeof(struct net_link));
            newlink->hostfd = newfd;
            newlink->next   = *all_links;
            *all_links      = newlink;
        }

        // Receive any messages incoming, then forward them to hosts;
        next_link = *all_links;
        while(next_link != NULL) {
            
            recv_size = recv(next_link->hostfd, buff, 100 + 5, 0);
            if(recv_size == 0) {
                printf("Deleting node from table and list\n");
                for(i = 0; i < TABLE_SIZE; ++i) {
                    if(table[i] == next_link) {
                        table[i] = NULL;
                        printf("Deleted node from table index %d\n", i);
                    }
                }
                remove_link(all_links, next_link);
                next_link = NULL;
                continue;
            }
            // If we received a packet, send to desired host OR send to all hosts
            else if(recv_size > 0) {
                int src, dst;
                char msg[100 + 1];
                sscanf(buff, "%d %d %[^\t\n]", &src, &dst, msg);
                printf("Received message!\n");
                printf("From %d to %d: %s\n", src, dst, msg);

                // Add to table if needed
                if(table[src] == NULL) {
                    printf("Adding user %d to table\n", src);
                    table[src] = next_link;
                }

                // Send to host or all if necessary
                if(table[dst] != NULL && dst != -1) {
                    printf("Sending message directly to host %d\n", dst);
                    send(table[dst]->hostfd, buff, recv_size, MSG_NOSIGNAL);
                }
                else {
                    printf("Sending message to all hosts\n");
                    struct net_link* send_link = *all_links;
                    while(send_link != NULL) {
                        // TODO: Add error checking
                        send(send_link->hostfd, buff, recv_size, MSG_NOSIGNAL);
                        send_link = send_link->next;
                    }
                }
            }
            next_link = next_link->next;
        }
        // Sleep for 50 ms, or 50000 us
        usleep(50000);
    }

    return 0;
}
