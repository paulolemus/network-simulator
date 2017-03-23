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
	SWITCH_JOB_SEND_ALL_PORTS,
    SWITCH_JOB_SEND_TO_PORT
};

struct switch_job {
        enum switch_job_type type;
        struct packet *packet;
        int in_port_index;
        int out_port_index;
        int file_upload_dst;
        struct switch_job *next;
};

struct switch_job_queue {
        struct switch_job *head;
        struct switch_job *tail;
        int occ;
};

struct table_node {
    char host_id;
    int port_num;
    table_node* left;
    table_node* right;
    char color;
};

struct forward_table {
    table_node* root;
    table_node* null;
    int size;
};


void switch_main(int switch_id);
