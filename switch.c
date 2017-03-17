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

