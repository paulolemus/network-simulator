/* Name: Jared Shimabukuro
 * UH ID: 2257-2949
 * Section: 001
 *
 * Teammates: Kevin Cabael, Paulo Lemus
 *
 * File Name: dns.c
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
#include "dns.h"

#define FIVEMILLISEC 5000
#define TABLE_SIZE   100
#define BACKLOG 10

/*
 * Operations requested by the manager
 */


/* Job queue operations */

/* Add a job to the job queue */
void dns_job_q_add(struct dns_job_queue *j_q, struct dns_job *j)
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
struct dns_job *dns_job_q_remove(struct dns_job_queue *j_q)
{
    struct dns_job *j;

    if (j_q->occ == 0) return(NULL);
    j = j_q->head;
    j_q->head = (j_q->head)->next;
    j_q->occ--;
    return(j);
}

/* Initialize job queue */
void dns_job_q_init(struct dns_job_queue *j_q)
{
    j_q->occ = 0;
    j_q->head = NULL;
    j_q->tail = NULL;
}

int dns_job_q_num(struct dns_job_queue *j_q)
{
    return j_q->occ;
}

void dns_main(int dns_id)
{

    struct net_port *node_port_list;
    struct net_port **node_port;	//Array of Pointers to Node Ports
    int node_port_num;		//Number of node ports

    int i, k, n;
    int dst;

    struct packet *in_packet; /* Incoming Packet */
    struct packet *new_packet;

    struct net_port *p;
    struct dns_job *new_job;
    struct dns_job *new_job2;

    struct dns_job_queue job_q;

    int ping_reply_received;

    /*
     * Create an array node_port[ ] to store the network link ports
     * at the host.  The number of ports is node_port_num
     */
    node_port_list = net_get_port_list(dns_id);

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
    dns_job_q_init(&job_q);

    while(1) {
        /* Receive packet from a host */
/*        for(k = 0; k < node_port_num; k++) {
            in_packet = (struct packet *) malloc(sizeof(struct packet));
            struct sockaddr_storage* their_addr = NULL;
            socklen_t addr_size;
            n = packet_recv(node_port[k], in_packet);
            if(n > 0) {
                
                // TODO: Determine where a packet came from if it came from a socket
                if(node_port[k]->type == SOCKET) {
                    if(their_addr != NULL) free(their_addr);
                }
                // Check if the src is in the table. If not, add the net_port

                printf("\ndns received a Packet!\n");
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
*/

	struct dns_list* domNam;
	//Put Jobs in Job Queue
	for (k = 0; k < node_port_num; k++) {
		in_packet = (struct packet*) malloc(sizeof(struct packet));
		n = packet_recv(node_port[k], in_packet);

		if((n > 0) && ((int) in_packet->dst == dns_id)) {
			new_job = (struct dns_job*)
				malloc(sizeof(struct dns_job));
			new_job->in_port_index = k;
			new_job->packet = in_packet;

			switch(in_packet->type) {
			case (char) PKT_PING_REQ:
				new_job->type = DNS_JOB_PING_SEND_REPLY;
				dns_job_q_add(&job_q, new_job);
				break;

			case (char) PKT_PING_REPLY:
				ping_reply_received = 1;
				free(in_packet);
				free(new_job);
				break;

			case (char) PKT_DNS_REGISTER:
				domNam->host_id = in_packet->src;
				for(i = 0; i < 100; i++) {
				domNam->domain[i] = in_packet->payload[i];
				}
				printf("Domain Name Registered!\n");
				break;

			case (char) PKT_DNS_LOOKUP:
				while(domNam != NULL) {
					*domNam = *domNam->next;

					if(in_packet->src == 
					   domNam->host_id) {
						printf("Domain Name of Host %d is %s.\n", domNam->host_id, domNam->domain);
						break;
					} 
				}	
				
				break;
			
			default:
				free(in_packet);
				free(new_job);
			}
		}

		else	free(in_packet);
	}

        // Execute one job in queue
        if(dns_job_q_num(&job_q) > 0) {
		new_job = dns_job_q_remove(&job_q);

		switch(new_job->type) {
                case DNS_JOB_SEND_PKT_ALL_PORTS:
                    for (k=0; k<node_port_num; k++) {
                        printf("\nDNS %d sending packet:\n", dns_id);
                        printf("src: %d\n", new_job->packet->src);
                        printf("dst: %d\n", new_job->packet->dst);
                        printf("type: %d\n", new_job->packet->type);
                        printf("length: %d\n", new_job->packet->length);
                        printf("payload: ");
                        int ii;
                        for(ii = 0; ii < new_job->packet->length; ++ii) {
                            printf("%c", new_job->packet->payload[ii]);
                        } printf("\n");
                        packet_send(node_port[k], new_job->packet);
                    }
                    free(new_job->packet);
                    free(new_job);
                    break;

		case DNS_JOB_PING_SEND_REPLY:
			new_packet = (struct packet* )
				malloc(sizeof(struct packet));
			new_packet->dst = new_job->packet->src;
			new_packet->src = (char) dns_id;
			new_packet->type = PKT_PING_REPLY;
			new_packet->length = 0;

			new_job2 = (struct dns_job *)
				malloc(sizeof(struct dns_job));
			new_job2->type = DNS_JOB_SEND_PKT_ALL_PORTS;
			new_job2->packet = new_packet;

			dns_job_q_add(&job_q, new_job2);

			free(new_job->packet);
			free(new_job);
			break;
		}

        }
        usleep(FIVEMILLISEC);
    } /* End while loop */
}
