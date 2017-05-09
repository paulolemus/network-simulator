
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "main.h"
#include "packet.h"
#include "net.h"
#include "host.h"


void packet_send(struct net_port *port, struct packet *p)
{
    char msg[PAYLOAD_MAX+4];
    int i;

    if (port->type == PIPE) {
        msg[0] = (char) p->src; 
        msg[1] = (char) p->dst;
        msg[2] = (char) p->type;
        msg[3] = (char) p->length;
        for (i = 0; i < p->length; i++) {
            msg[i + 4] = p->payload[i];
        }
        write(port->pipe_send_fd, msg, p->length+4);
        printf("PACKET SEND, src=%d dst=%d p-src=%d p-dst=%d\n", 
                (int) msg[0], 
                (int) msg[1], 
                (int) p->src, 
                (int) p->dst);
    }

    else if (port->type == SOCKET) {
        printf("Entered socket send\n");
        msg[0] = (char) p->src; 
        msg[1] = (char) p->dst;
        msg[2] = (char) p->type;
        msg[3] = (char) p->length;
        for (i = 0; i < p->length; i++) {
            msg[i + 4] = p->payload[i];
        }
        int              newfd;
        struct addrinfo  hints;
        struct addrinfo* servinfo;
        struct addrinfo* ptr;
        memset(&hints, 0, sizeof hints);
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        if(getaddrinfo(port->connect_addr, port->connect_port, &hints, &servinfo) != 0) {
            printf("Get addr info in send failed\n");
            return;
        }

        for(ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
            if((newfd = socket(ptr->ai_family, 
                               ptr->ai_socktype, 
                               ptr->ai_protocol)) == -1) {
                perror("Client connect error in send\n");
                continue;
            }
            if(connect(newfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
                close(newfd);
                perror("send failed to connect\n");
                continue;
            }
            break;
        }
        if(ptr == NULL) {
            printf("Failed to connect while sending packet\n");
            return;
        }
        freeaddrinfo(servinfo);
        fcntl(newfd, F_SETFL, O_NONBLOCK);
        send(newfd, msg, p->length+4, 0);
        printf("PACKET SEND, src=%d dst=%d p-src=%d p-dst=%d\n", 
                (int) msg[0], 
                (int) msg[1], 
                (int) p->src, 
                (int) p->dst);
        close(newfd);
    }
    return;
}

int packet_recv(struct net_port *port, struct packet *p)
{
    char msg[PAYLOAD_MAX+4];
    int n;
    int i;

    if (port->type == PIPE) {
        n = read(port->pipe_recv_fd, msg, PAYLOAD_MAX+4);
        if (n>0) {
            p->src    = (char) msg[0];
            p->dst    = (char) msg[1];
            p->type   = (char) msg[2];
            p->length = (int) msg[3];
            for (i=0; i<p->length; i++) {
                p->payload[i] = msg[i+4];
            }

            printf("PACKET RECV, src=%d dst=%d p-src=%d p-dst=%d\n", 
             	(int) msg[0], 
             	(int) msg[1], 
             	(int) p->src, 
             	(int) p->dst);
        }
    }

    else if(port->type == SOCKET) {

        // connect to incoming connections
        struct sockaddr_storage their_addr;
        socklen_t addr_size;
        int newfd = accept(port->sock_recv_fd, (struct sockaddr*)&their_addr, &addr_size);

        if(newfd > 0) {
            fcntl(newfd, F_SETFL, O_NONBLOCK);
            n = recv(newfd, msg, PAYLOAD_MAX + 4, 0);
            if(n > 0) {
                p->src    = (char) msg[0];
                p->dst    = (char) msg[1];
                p->type   = (char) msg[2];
                p->length = (int) msg[3];
                for (i = 0; i < p->length; i++) {
                    p->payload[i] = msg[i + 4];
                }

                 printf("PACKET RECV, src=%d dst=%d p-src=%d p-dst=%d\n", 
                        (int) msg[0], 
                        (int) msg[1], 
                        (int) p->src, 
                        (int) p->dst);
            }
        }
    }
    return n;
}


