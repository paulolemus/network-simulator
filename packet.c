
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
    char choice;

/*    printf("Is the packet a tree packet? (y)es or (n)o: ");
    scanf("%c", &choice);

    if(choice == 'y') {
	p->ptype = TREE;
    }

    else	p->ptype = STANDARD;
*/
    p->ptype = TREE;
    if (port->type == PIPE) {
        msg[0] = (char) p->src; 
        msg[1] = (char) p->dst;
        msg[2] = (char) p->type;
        msg[3] = (char) p->length;
	msg[4] = (char) p->ptype;
	msg[5] = (char) p->packetRootID;
	msg[6] = (char) p->packetRootDist;
	msg[7] = (char) p->packetSenderType;
	msg[8] = (char) p->packetSenderChild;
        for (i=0; i<p->length; i++) {
            msg[i+9] = p->payload[i];
//        for (i = 0; i < p->length; i++) {
//            msg[i + 4] = p->payload[i];
        }
        write(port->pipe_send_fd, msg, p->length+9);
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
	msg[4] = (char) p->ptype;
        msg[5] = (char) p->packetRootID;
        msg[6] = (char) p->packetRootDist;
        msg[7] = (char) p->packetSenderType;
        msg[8] = (char) p->packetSenderChild;
        for (i=0; i<p->length; i++) {
            msg[i+9] = p->payload[i];
        }
        send(port->sock_send_fd, msg, p->length+9, 0);
//        for (i = 0; i < p->length; i++) {
//             msg[i + 4] = p->payload[i];
//        }
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
    char msg[PAYLOAD_MAX+9];
    int n;
    int i;

    if (port->type == PIPE) {
        n = read(port->pipe_recv_fd, msg, PAYLOAD_MAX+9);
        if (n>0) {
            p->src    = (char) msg[0];
            p->dst    = (char) msg[1];
            p->type   = (char) msg[2];
            p->length = (int) msg[3];
	    p->ptype = (enum PacketType) msg[4];
	    p->packetRootID = (int) msg[5];
	    p->packetRootDist = (int) msg[6];
	    p->packetSenderType = (char) msg[7];
	    p->packetSenderChild = (char) msg[8];
            for (i=0; i<p->length; i++) {
                p->payload[i] = msg[i+9];
            }

            printf("PACKET RECV, src=%d dst=%d p-src=%d p-dst=%d\n", 
             	(int) msg[0], 
             	(int) msg[1], 
             	(int) p->src, 
             	(int) p->dst);
        }
    }

    else if(port->type == SOCKET) {
//<<<<<<< HEAD
        n = recv(port->sock_recv_fd, msg, PAYLOAD_MAX+9, 0);
        if(n > 0) {
            p->src = (char) msg[0];
            p->dst = (char) msg[1];
            p->type = (char) msg[2];
//=======

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


int switch_packet_recv(struct net_port *port, struct packet *p,
                       struct sockaddr_storage** addr, socklen_t* addr_size)
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
//>>>>>>> master
            p->length = (int) msg[3];
	    p->ptype = (enum PacketType) msg[4];
            p->packetRootID = (int) msg[5];
            p->packetRootDist = (int) msg[6];
            p->packetSenderType = (char) msg[7];
            p->packetSenderChild = (char) msg[8];

            for (i=0; i<p->length; i++) {
                p->payload[i] = msg[i+9];
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
        *addr = (struct sockaddr_storage*)malloc(sizeof(struct sockaddr_storage));
        int newfd = accept(port->sock_recv_fd, (struct sockaddr*)*addr, addr_size);

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
        else {
            free(*addr);
            *addr = NULL;
        }
    }
    return n;
}


