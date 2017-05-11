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

    // DOMAIN LIST STUFF
    struct dns_list* domain_list = NULL;
    struct dns_list* curr_node   = NULL;
    int found_in_list;
    char domain_name_sp[100];
    int found_host_id;

    while(1) {

	//Put Jobs in Job Queue
	for (k = 0; k < node_port_num; k++) {
		in_packet = (struct packet*) malloc(sizeof(struct packet));
		n = packet_recv(node_port[k], in_packet);


		if((n > 0) && ((int) in_packet->dst == dns_id)) {
            printf("DNS received packet\n");
            printf("src: %d\n", in_packet->src);
            printf("dst: %d\n", in_packet->dst);
            printf("type: %d\n", in_packet->type);
            printf("src: %d\n", in_packet->length);
            for(int jj = 0; jj < in_packet->length; ++jj) {
                printf("%c", in_packet->payload[jj]);
            }
            printf("\n");
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
    
                curr_node = (struct dns_list*)malloc(sizeof(struct dns_list));
                curr_node->host_id = in_packet->src;
                for(int jj = 0; jj < in_packet->length; ++jj) {
                    curr_node->domain[jj] = in_packet->payload[jj];
                }
                curr_node->domain[in_packet->length] = '\0';
                curr_node->next = domain_list;
                domain_list = curr_node;
                printf("Registered your domain!\n");
				break;

			case (char) PKT_DNS_LOOKUP:
                // Find name then send a message to source
                // with the corresponding host id;
                found_host_id = -1;
                for(int jj = 0; jj < in_packet->length; ++jj) {
                    domain_name_sp[jj] = in_packet->payload[jj];
                }
                domain_name_sp[in_packet->length] = '\0';
                curr_node = domain_list;
                while(curr_node && found_host_id < 0) {
                    if(strcmp(domain_name_sp, curr_node->domain) == 0) {
                        found_host_id = curr_node->host_id;
                    }
                    curr_node = curr_node->next;
                }
                in_packet->dst = in_packet->src;
                in_packet->src = (char) 100;
                in_packet->length = 1;
                in_packet->payload[0] = (char) 1; //found_host_id
                printf("DNS SENDING\n");
                printf("src: %d\n", in_packet->src);
                printf("dst: %d\n", in_packet->dst);
                printf("type: %d\n", in_packet->type);
                printf("length: %d\n", in_packet->length);
                printf("content: %c\n", in_packet->payload[0]);
                packet_send(node_port_list, in_packet);
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
