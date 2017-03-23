/* Name: Jared Shimabukuro
 * UH ID: 2257-2949
 * Section: 001
 *
 * Teammates: Kevin Cabael, Paulo Lemus
 *
 * File Name: switch.h
 * Description: Relay packets from one host to another
 *
 * Date Started: 16 March 2017
 * Date Finished: 
 */

enum switch_job_type {
	JOB_SEND_PKT_ALL_PORTS,
    JOB_SEND_TO_PORT
};

struct switch_job {
        enum switch_job_type type;
        struct packet *packet;
        int in_port_index;
        int out_port_index;
        int file_upload_dst;
        struct host_job *next;
};

struct job_queue {
        struct host_job *head;
        struct host_job *tail;
        int occ;
};


void switch_main(int switch_id);
