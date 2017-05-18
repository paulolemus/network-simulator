/* Name: Jared Shimabukuro
 * UH ID: 2257-2949
 * Section: 001
 *
 * Teammates: Kevin Cabael, Paulo Lemus
 *
 * File Name: switch.c
 * Description: Relay packets from one host to another
 *
 * Date Started: 16 March 2017
 * Date Finished:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "net.h"
#include "man.h"
#include "packet.h"
#include "switch.h"

#define FIVEMILLISEC 5000
#define TABLE_SIZE   100
#define BACKLOG 10

/*
 * Operations requested by the manager
 */


/* Job queue operations */

/* Add a job to the job queue */
void switch_job_q_add(struct switch_job_queue *j_q, struct switch_job *j)
{
    if (j_q->head == NULL ) {
        j->next = NULL;
        j_q->head = j;
        j_q->tail = j;
        j_q->occ = 1;
    }
    else {
        (j_q->tail)->next = j;
        j->next = NULL;
        j_q->tail = j;
        j_q->occ++;
    }
}

/* Remove job from the job queue, and return pointer to the job*/
struct switch_job *switch_job_q_remove(struct switch_job_queue *j_q)
{
    struct switch_job *j;

    if (j_q->occ == 0) return(NULL);
    j = j_q->head;
    j_q->head = (j_q->head)->next;
    j_q->occ--;
    return(j);
}

/* Initialize job queue */
void switch_job_q_init(struct switch_job_queue *j_q)
{
    j_q->occ = 0;
    j_q->head = NULL;
    j_q->tail = NULL;
}

int switch_job_q_num(struct switch_job_queue *j_q)
{
    return j_q->occ;
}

struct net_port** init_table(struct net_port** list) 
{
    struct net_port** table = (struct net_port **)
        malloc(TABLE_SIZE * sizeof(struct net_port*));

    for(int i = 0; i < TABLE_SIZE; ++i) table[i] = NULL;
    return table;
}


void switch_main(int switch_id)
{

    struct net_port *node_port_list;
    struct net_port **node_port;	//Array of Pointers to Node Ports
    int node_port_num;		//Number of node ports

    int i, k, n;
    int dst;

    struct packet *in_packet; /* Incoming Packet */
    struct packet *new_packet;

    struct net_port *p;
    struct switch_job *new_job;
    struct switch_job *new_job2;

    struct switch_job_queue job_q;

    /*
     * Create an array node_port[ ] to store the network link ports
     * at the host.  The number of ports is node_port_num
     */
    node_port_list = net_get_port_list(switch_id);

    /*  Count the number of network link ports */
    node_port_num = 0;
    for (p=node_port_list; p!=NULL; p=p->next) {
        node_port_num++;
    }
    /* Create memory space for the array */
    node_port = (struct net_port **) malloc(node_port_num*sizeof(struct net_port *));
    /* Load ports into the array */
    p = node_port_list;
    for (k = 0; k < node_port_num; k++) {
        node_port[k] = p;
        p = p->next;
    }

    // Create an array of pointers to net_port structs,
    // with host id as the index
    struct net_port** table = init_table(node_port);

    //////////////////////////////
    // FD STUFF - WE LISTEN TO ONE PORT
    int listenfd = -1, newfd = -1;

    p = node_port_list;
    while(p && p->type != SOCKET) p = p->next;
    if(p && p->type == SOCKET) {
        struct addrinfo hints;
        struct addrinfo* servinfo;
        memset(&hints, 0, sizeof hints);
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        if((getaddrinfo(p->listen_addr, p->listen_port, &hints, &servinfo)) != 0) {
            printf("socket getaddrinfo failed\n");
            exit(1);
        }
        int yes = 1;
        struct addrinfo* cp;
        for(cp = servinfo; cp != NULL; cp = cp->ai_next) {
            if((listenfd = socket(cp->ai_family, 
                                  cp->ai_socktype, 
                                  cp->ai_protocol)) < 0) {
                continue;
            }
            fcntl(listenfd, F_SETFL, O_NONBLOCK);
            if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
                exit(1);
            }
            if(bind(listenfd, cp->ai_addr, cp->ai_addrlen) < 0) {
                close(listenfd);
                continue;
            }
            break;
        }
        if(cp == NULL) {
            printf("FAILED TO BIND TO PORT IN SWITCH\n");
            exit(1);
        }
        freeaddrinfo(servinfo);
        if(listen(listenfd, BACKLOG) < 0) {
            printf("socket listen failed\n");
            exit(1);
        }
        while(p) {
            if(p->type == SOCKET) {
                p->sock_recv_fd = listenfd;
            }
            p = p->next;
        }
    }
    ////////////////////////////////////

    /* Initialize the job queue */
    switch_job_q_init(&job_q);

    while(1) {
        /* Receive packet from a host */
        for(k = 0; k < node_port_num; k++) {
            in_packet = (struct packet *) malloc(sizeof(struct packet));
            struct sockaddr_storage* their_addr = NULL;
            socklen_t addr_size;
            n = switch_packet_recv(node_port[k], in_packet, &their_addr, &addr_size);
            if(n > 0) {
                
                // TODO: Determine where a packet came from if it came from a socket
                if(node_port[k]->type == SOCKET) {
                    if(their_addr != NULL) free(their_addr);
                }
                // Check if the src is in the table. If not, add the net_port
                else if((int)in_packet->src >= 0         &&
                        (int)in_packet->src < TABLE_SIZE &&
                        table[in_packet->src] == NULL) {

                    table[in_packet->src] = node_port[k];
                }

                printf("\nSwitch received a Packet!\n");
                printf("src: %d \n", in_packet->src);
                printf("dst: %d \n", in_packet->dst);
                printf("type: %d \n", in_packet->type);
                printf("length: %d \n", in_packet->length);
                printf("payload: ");
                for(i = 0; i < in_packet->length; ++i) {
                    printf("%c", in_packet->payload[i]);
                } printf("\n");

                
                // Check table to see if we have the dest FD
                if(table[in_packet->dst] != NULL) {
                    printf("Sending to specific FD\n");
                    packet_send(table[in_packet->dst], in_packet);
                }
                else {
                    printf("sending to all FDs\n");
                    for(i = 0; i < node_port_num; i++){
                        packet_send(node_port[i], in_packet);
                    }
                }
            }
            free(in_packet);
        } // for loop - receive packets

        // Execute one job in queue
        if(switch_job_q_num(&job_q) > 0) {

        }
        usleep(FIVEMILLISEC);
    } /* End while loop */
}
