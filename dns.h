/* 
 * dns.h 
 */

enum dns_job_type {
	DNS_JOB_SEND_PKT_ALL_PORTS,
	DNS_JOB_PING_SEND_REPLY
};

struct dns_job {
	enum dns_job_type type;
	struct packet *packet;
	int in_port_index;
	int out_port_index;
	char fname_download[100];
	char fname_upload[100];
	int ping_timer;
	struct dns_job *next;
};


struct dns_job_queue {
	struct dns_job *head;
	struct dns_job *tail;
	int occ;
};

struct dns_list {
	int host_id;
	char domain[100];
	struct dns_list* next;
};

void dns_main(int dns_id);


