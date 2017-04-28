
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

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
        printf("PACKET SEND, src=%d dst=%d p-src=%d p-dst=%d\n", 
                (int) msg[0], 
                (int) msg[1], 
                (int) p->src, 
                (int) p->dst);
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
            p->src = (char) msg[0];
            p->dst = (char) msg[1];
            p->type = (char) msg[2];
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
        n = recv(port->sock_recv_fd, msg, PAYLOAD_MAX+9, 0);
        if(n > 0) {
            p->src = (char) msg[0];
            p->dst = (char) msg[1];
            p->type = (char) msg[2];
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

    return(n);
}


