/* File: client.c
 * Name: Paulo Lemus
 * Date: 4/6/2017
 */

/* This program contains code for a single host.
 * A host can connect to a remote server and chat with
 * other hosts on the server.
 *
 * It sends and receives messages from
 * the single link to the server.
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


int main(int argc, char** argv) {

    int i, j, k;    // used for all forloops
    int sockfd;     // The main fdd used for communication
    int yes = 1;    // Yes
    char s0[INET6_ADDRSTRLEN]; // the address connected to

    int hostid;
    char domain[MAX_DOMAIN_NAME]; // domain read from argv
    int tcp;                      // The port number to listen

    struct addrinfo  hints;    // used to select port config options
    struct addrinfo* servinfo; // pointer to results of setup


    // Ensure user has a entered possible address and port
    if(argc != 4) {
        printf("Need to execute with hostID, address and port of server\n");
        printf("Example: ./a.out 1 127.0.0.1 3001\n");
        exit(1);
    }

    // read host id
    sscanf(argv[1], "%d", &hostid);
    // read address
    sscanf(argv[2], "%s", domain);
    // read port
    sscanf(argv[3], "%d", &tcp);
    // print to check
    printf("Host id:    : %d\n", hostid);
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
    if((getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0) {
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
        break;
    }

    // Make the actual socket using info in servinfo
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    printf("SockFD: %d\n", sockfd);

    // Set socket options, allow other sockets to bind to port
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    // Set socket to non blocking
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    // Connect to the server
    printf("About to connect\n");
    while(connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
        printf("Failed to connect, trying again in 3");
        usleep(1000000);
        printf(" 2");
        usleep(1000000);
        printf(" 1....\n");
        usleep(1000000);
    }
    printf("Connected!\n");

    // Once used, delete the linked list
    freeaddrinfo(servinfo);

    //////////////////////////////////////////////////////////////////


    struct sockaddr_storage their_addr; // Used to get info about new connections
    socklen_t addr_size;                // Get more info from new connection

    char buff[100 + 7]; // 100 chars + 3 for host, 3 for destination, 1 for '\0'
    int recv_size;

    srand(time(NULL));
    int  sleepTime  = 50000;
    int  nextHost   = 0;
    int  printCount = 0;
    int  dest;
    int  hostMsgSize;
    char hostMsg[100];

    printf("Entered main while loop\n");
    while(1) {

        recv_size = recv(sockfd, buff, 100 + 5, 0);
        // If we received a packet, print it
        if(recv_size > 0) {
            int src, dst;
            char msg[100 + 1];
            sscanf(buff, "%d %d %[^\t\n]", &src, &dst, msg);

            if(dst == hostid || dst == -1) {
                printf("Received from host %d: ", src);
                printf("%s\n", msg);
            }
            memset(buff, 0, sizeof buff);

        }

        printCount += rand() % 10;

        if(printCount > 500) {
            dest = rand() % 11 - 1;
            hostMsgSize = sprintf(hostMsg, 
                                  "%d %d Greetings from host %d!\n", 
                                  hostid, dest, hostid);
            //printf("Entered send, sending %s with size %d\n", hostMsg, hostMsgSize);


            send(sockfd, hostMsg, hostMsgSize, 0);
            printCount = 0;
        }

        // Sleep for 50 ms, or 50000 us
        sleepTime = rand() % 5000 + 50000;
        usleep(sleepTime);
    }

    return 0;
}
