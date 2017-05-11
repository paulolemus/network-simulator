/* 
 * dns.h 
 */

enum dns_job_type {
	DNS_JOB_SEND_PKT_ALL_PORTS,
	DNS_JOB_PING_SEND_REQ,	
	DNS_JOB_PING_SEND_REPLY,
	DNS_JOB_PING_WAIT_FOR_REPLY,
	DNS_JOB_FILE_UPLOAD_SEND,
	DNS_JOB_FILE_UPLOAD_RECV_START,
   DNS_JOB_FILE_UPLOAD_RECV_IN, // Added this line
	DNS_JOB_FILE_UPLOAD_RECV_END
};

struct dns_job {
	enum dns_job_type type;
	struct packet *packet;
	int in_port_index;
	int out_port_index;
	char fname_download[100];
	char fname_upload[100];
	int ping_timer;
	int file_upload_dst;
	struct dns_job *next;
};


struct dns_job_queue {
	struct dns_job *head;
	struct dns_job *tail;
	int occ;
};

void dns_main(int dns_id);


