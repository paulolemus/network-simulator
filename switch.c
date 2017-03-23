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

#include "switch.h"

/*
 * Operations with the manager
 */

int get_man_command(struct man_port_at_host *port, char msg[], char *c) {

int n;
int i;
int k;

n = read(port->recv_fd, msg, MAN_MSG_LENGTH); /* Get command from manager */
if (n>0) {  /* Remove the first char from "msg" */
        for (i=0; msg[i]==' ' && i<n; i++);
        *c = msg[i];
        i++;
        for (; msg[i]==' ' && i<n; i++);
        for (k=0; k+i<n; k++) {
                msg[k] = msg[k+i];
        }
        msg[k] = '\0';
}
return n;

}

/*
 * Operations requested by the manager
 */


/* Job queue operations */

/* Add a job to the job queue */
void job_q_add(struct job_queue *j_q, struct switch_job *j)
{
if (j_q->head == NULL ) {
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
struct switch_job *job_q_remove(struct job_queue *j_q)
{
struct switch_job *j;

if (j_q->occ == 0) return(NULL);
j = j_q->head;
j_q->head = (j_q->head)->next;
j_q->occ--;
return(j);
}

/* Initialize job queue */
void job_q_init(struct job_queue *j_q)
{
j_q->occ = 0;
j_q->head = NULL;
j_q->tail = NULL;
}

int job_q_num(struct job_queue *j_q)
{
return j_q->occ;
}


void switch_main(int switch_id)
{
char dir[MAX_DIR_NAME];
int dir_valid = 0;

struct net_port *node_port_list;
struct net_port **node_port;	//Array of Pointers to Node Ports
int node_port_num;		//Number of node ports

int ping_reply_received;

int i, k, n;
int dst;
char name[MAX_FILE_NAME];
char string[PKT_PAYLOAD_MAX+1];

FILE *fp;

struct packet *in_packet; /* Incoming Packet */
struct packet *new_packet;

struct net_port *p;
struct switch_job *new_job;
struct switch_job *new_job2;

struct job_queue job_q;

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
node_port = (struct net_port **)
        malloc(node_port_num*sizeof(struct net_port *));

        /* Load ports into the array */
p = node_port_list;
for (k = 0; k < node_port_num; k++) {
        node_port[k] = p;
        p = p->next;
}

/* Initialize the job queue */
job_q_init(&job_q);

while(1) {
	/* Receive packet from a host */
	for(k = 0; k < node_port_num; k++) {
		in_packet = (struct packet *) malloc(sizeof(struct packet));
		n = packet_recv(node_port[k], in_packet);
		if(n > 0) {
//			new_job = (struct switch_job *)
//				malloc(sizeof(struct switch_job));
//			new_job->in_port_index = k;
//			new_job->packet = in_packet;

			for(i = 0; i < node_port_num; i++){
				packet_send(node_port[k], in_packet);
			}

			free(in_packet);
		}
		
		else	free(in_packet);
	}		
}
