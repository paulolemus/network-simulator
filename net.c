/*
 * net.c
 *
 * Here is where pipes and sockets are created.
 * Note that they are "nonblocking".  This means that
 * whenever a read/write (or send/recv) call is made,
 * the called function will do its best to fulfill
 * the request and quickly return to the caller.
 *
 * Note that if a pipe or socket is "blocking" then
 * when a call to read/write (or send/recv) will be blocked
 * until the read/write is completely fulfilled.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>

#include "main.h"
#include "man.h"
#include "host.h"
#include "net.h"
#include "packet.h"


#define MAX_FILE_NAME 100
#define PIPE_READ 0
#define PIPE_WRITE 1

#define SOCKET_RECV 0
#define SOCKET_SEND 1
#define BACKLOG 10

#define MAX_DOMAIN_NAME 100

enum bool {FALSE, TRUE};

/* 
 * Struct used to store a link. It is used when the 
 * network configuration file is loaded.
 */

struct net_link {
    enum NetLinkType type;
    int pipe_node0;
    int pipe_node1;
    
    int socket_node;
    char socket_domain0[MAX_DOMAIN_NAME];
    int socket_tcp0;
    char socket_domain1[MAX_DOMAIN_NAME];
    int socket_tcp1;
    
};

/* 
 * The following are private global variables to this file net.c
 */
static enum bool g_initialized = FALSE; /* Network initialized? */  
/* The network is initialized only once */

/* 
 * g_net_node[] and g_net_node_num have link information from
 * the network configuration file.
 * g_node_list is a linked list version of g_net_node[]
 */
static struct net_node *g_net_node;
static int g_net_node_num;
static struct net_node *g_node_list = NULL;

/* 
 * g_net_link[] and g_net_link_num have link information from
 * the network configuration file
 */
static struct net_link *g_net_link;
static int g_net_link_num;

/* 
 * Global private variables about ports of network node links
 * and ports of links to the manager
 */
static struct net_port *g_port_list = NULL;

static struct man_port_at_man *g_man_man_port_list = NULL;
static struct man_port_at_host *g_man_host_port_list = NULL;

/* 
 * Loads network configuration file and creates data structures
 * for nodes and links.  The results are accessible through
 * the private global variables
 */
int load_net_data_file();

/*
 * Creates a data structure for the nodes
 */
void create_node_list();

/*
 * Creates links, using pipes
 * Then creates a port list for these links.
 */
void create_port_list();

/*
 * Creates ports at the manager and ports at the hosts so that
 * the manager can communicate with the hosts.  The list of
 * ports at the manager side is p_m.  The list of ports
 * at the host side is p_h.
 */
void create_man_ports(
        struct man_port_at_man **p_m, 
        struct man_port_at_host **p_h);

void net_close_man_ports_at_hosts();
void net_close_man_ports_at_hosts_except(int host_id);
void net_free_man_ports_at_hosts();
void net_close_man_ports_at_man();
void net_free_man_ports_at_man();

/*
 * Get the list of ports for host host_id
 */
struct net_port *net_get_port_list(int host_id);

/*
 * Get the list of nodes
 */
struct net_node *net_get_node_list();



/*
 * Remove all the ports for the host from linked lisst g_port_list.
 * and create another linked list.  Return the pointer to this
 * linked list.
 */
struct net_port *net_get_port_list(int host_id)
{

    struct net_port **p;
    struct net_port *r;
    struct net_port *t;

    r = NULL;
    p = &g_port_list;

    while (*p != NULL) {
        if ((*p)->pipe_host_id == host_id) {
            t = *p;	
            *p = (*p)->next;
            t->next = r;
            r = t;
        }
        else {
            p = &((*p)->next);
        }
    }

    return r;
}

/* Return the linked list of nodes */
struct net_node *net_get_node_list()
{
    return g_node_list;
}

/* Return linked list of ports used by the manager to connect to hosts */
struct man_port_at_man *net_get_man_ports_at_man_list()
{
    return(g_man_man_port_list); 
}

/* Return the port used by host to link with other nodes */
struct man_port_at_host *net_get_host_port(int host_id)
{
    struct man_port_at_host *p;

    for (p = g_man_host_port_list;
            p != NULL && p->host_id != host_id;
            p = p->next);

    return(p);
}


/* Close all host ports not used by manager */
void net_close_man_ports_at_hosts()
{
    struct man_port_at_host *p_h;

    p_h = g_man_host_port_list;

    while (p_h != NULL) {
        close(p_h->send_fd);
        close(p_h->recv_fd);
        p_h = p_h->next;
    }
}

/* Close all host ports used by manager except to host_id */
void net_close_man_ports_at_hosts_except(int host_id)
{
    struct man_port_at_host *p_h;

    p_h = g_man_host_port_list;

    while (p_h != NULL) {
        if (p_h->host_id != host_id) {
            close(p_h->send_fd);
            close(p_h->recv_fd);
        }
        p_h = p_h->next;
    }
}

/* Free all host ports to manager */
void net_free_man_ports_at_hosts()
{
    struct man_port_at_host *p_h;
    struct man_port_at_host *t_h;

    p_h = g_man_host_port_list;

    while (p_h != NULL) {
        t_h = p_h;
        p_h = p_h->next;
        free(t_h);
    }
}

/* Close all manager ports */
void net_close_man_ports_at_man()
{
    struct man_port_at_man *p_m;

    p_m = g_man_man_port_list;

    while (p_m != NULL) {
        close(p_m->send_fd);
        close(p_m->recv_fd);
        p_m = p_m->next;
    }
}

/* Free all manager ports */
void net_free_man_ports_at_man()
{
    struct man_port_at_man *p_m;
    struct man_port_at_man *t_m;

    p_m = g_man_man_port_list;

    while (p_m != NULL) {
        t_m = p_m;
        p_m = p_m->next;
        free(t_m);
    }
}


/* Initialize network ports and links */
int net_init()
{
    if (g_initialized == TRUE) { /* Check if the network is already initialized */
        printf("Network already loaded\n");
        return(0);
    }		
    else if (load_net_data_file()==0) { /* Load network configuration file */
        return(0);
    }
    /* 
     * Create a linked list of node information at g_node_list 
     */
    create_node_list();

    /* 
     * Create pipes and sockets to realize network links
     * and store the ports of the links at g_port_list
     */
    create_port_list();

    /* 
     * Create pipes to connect the manager to hosts
     * and store the ports at the host at g_man_host_port_list
     * as a linked list
     * and store the ports at the manager at g_man_man_port_list
     * as a linked list
     */
    create_man_ports(&g_man_man_port_list, &g_man_host_port_list);
}

/*
 *  Create pipes to connect the manager to host nodes.
 *  (Note that the manager is not connected to switch nodes.)
 *  p_man is a linked list of ports at the manager.
 *  p_host is a linked list of ports at the hosts.
 *  Note that the pipes are nonblocking.
 */
void create_man_ports(
        struct man_port_at_man **p_man, 
        struct man_port_at_host **p_host)
{
    struct net_node *p;
    int fd0[2];
    int fd1[2];
    struct man_port_at_man *p_m;
    struct man_port_at_host *p_h;
    int host;


    for (p=g_node_list; p!=NULL; p=p->next) {
        if (p->type == HOST) {
            p_m = (struct man_port_at_man *) 
                malloc(sizeof(struct man_port_at_man));
            p_m->host_id = p->id;

            p_h = (struct man_port_at_host *) 
                malloc(sizeof(struct man_port_at_host));
            p_h->host_id = p->id;

            pipe(fd0); /* Create a pipe */
            /* Make the pipe nonblocking at both ends */
            fcntl(fd0[PIPE_WRITE], F_SETFL, 
                    fcntl(fd0[PIPE_WRITE], F_GETFL) | O_NONBLOCK);
            fcntl(fd0[PIPE_READ], F_SETFL, 
                    fcntl(fd0[PIPE_READ], F_GETFL) | O_NONBLOCK);
            p_m->send_fd = fd0[PIPE_WRITE];
            p_h->recv_fd = fd0[PIPE_READ];

            pipe(fd1); /* Create a pipe */
            /* Make the pipe nonblocking at both ends */
            fcntl(fd1[PIPE_WRITE], F_SETFL, 
                    fcntl(fd1[PIPE_WRITE], F_GETFL) | O_NONBLOCK);
            fcntl(fd1[PIPE_READ], F_SETFL, 
                    fcntl(fd1[PIPE_READ], F_GETFL) | O_NONBLOCK);
            p_h->send_fd = fd1[PIPE_WRITE];
            p_m->recv_fd = fd1[PIPE_READ];

            p_m->next = *p_man;
            *p_man = p_m;

            p_h->next = *p_host;
            *p_host = p_h;
        }
    }	

}

/* Create a linked list of nodes at g_node_list */
void create_node_list()
{
    struct net_node *p;
    int i;

    g_node_list = NULL;
    for (i=0; i<g_net_node_num; i++) {
        p = (struct net_node *) malloc(sizeof(struct net_node));
        p->id = i;
        p->type = g_net_node[i].type;
        p->next = g_node_list;
        g_node_list = p;
    }

}

void sigchld_handler(int s) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

//get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *) sa)
			  ->sin_addr);
	}

	return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

/*
 * Create links, each with either a pipe or socket.
 * It uses private global varaibles g_net_link[] and g_net_link_num
 */
void create_port_list()
{
    struct net_port *p0;
    struct net_port *p1;
    struct net_port *sock;
    int node0, node1;
    int snode;
    int fd01[2];
    int fd10[2];
    int sockfd0, sockfd1;
    int newsockfd;
    int i;
    int yes = 1;
    struct sigaction sa;
    char s0[INET6_ADDRSTRLEN];
    char s1[INET6_ADDRSTRLEN];

    struct sockaddr_storage their_addr;
    socklen_t sin_size;

    struct addrinfo hints, *servinfo, *ps0, *ps1;
    int rv;

    g_port_list = NULL;
    for (i=0; i<g_net_link_num; i++) {
        if (g_net_link[i].type == PIPE) {

            node0 = g_net_link[i].pipe_node0;
            node1 = g_net_link[i].pipe_node1;

            p0 = (struct net_port *) malloc(sizeof(struct net_port));
            p0->type = g_net_link[i].type;
            p0->pipe_host_id = node0;

            p1 = (struct net_port *) malloc(sizeof(struct net_port));
            p1->type = g_net_link[i].type;
            p1->pipe_host_id = node1;

            pipe(fd01);  /* Create a pipe */
            /* Make the pipe nonblocking at both ends */
            fcntl(fd01[PIPE_WRITE], F_SETFL, 
                    fcntl(fd01[PIPE_WRITE], F_GETFL) | O_NONBLOCK);
            fcntl(fd01[PIPE_READ], F_SETFL, 
                    fcntl(fd01[PIPE_READ], F_GETFL) | O_NONBLOCK);
            p0->pipe_send_fd = fd01[PIPE_WRITE]; 
            p1->pipe_recv_fd = fd01[PIPE_READ]; 

            pipe(fd10);  /* Create a pipe */
            /* Make the pipe nonblocking at both ends */
            fcntl(fd10[PIPE_WRITE], F_SETFL, 
                    fcntl(fd10[PIPE_WRITE], F_GETFL) | O_NONBLOCK);
            fcntl(fd10[PIPE_READ], F_SETFL, 
                    fcntl(fd10[PIPE_READ], F_GETFL) | O_NONBLOCK);
            p1->pipe_send_fd = fd10[PIPE_WRITE]; 
            p0->pipe_recv_fd = fd10[PIPE_READ]; 

            p0->next = p1; /* Insert ports in linked lisst */
            p1->next = g_port_list;
            g_port_list = p0;

        }
        else if (g_net_link[i].type == SOCKET) { 
	   
            // Initialize Remote Socket Address Info
            printf("Initializing sockfd0 address\n");
            memset(&hints, 0, sizeof hints);
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;
	    hints.ai_flags = AI_PASSIVE;
		
	    char str_tcp0[4];
	    sprintf(str_tcp0, "%d", g_net_link[i].socket_tcp0);
	    getaddrinfo(g_net_link[i].socket_domain0, 
			str_tcp0, &hints, &servinfo);
	    printf("%d\n", g_net_link[i].socket_tcp0);
	    printf("%s\n", str_tcp0);

	    for(ps0 = servinfo; ps0 != NULL; ps0 = ps0->ai_next) {
		//Initialize Socket
		printf("Initializing sockfd0\n");
            	sockfd0 = socket(ps0->ai_family, ps0->ai_socktype,
				ps0->ai_protocol);
	
		//Set Socket Opt
		printf("Setting sockopt\n");
		setsockopt(sockfd0, SOL_SOCKET, SO_REUSEADDR, 
			   &yes, sizeof(int));	

	    	//Bind Socket to TCP/IP Address
	    	printf("Binding sockfd0\n");
	    	bind(sockfd0, ps0->ai_addr, ps0->ai_addrlen);

		break;
	    }


	    //Set Socket to Nonblocking
	    printf("Setting sockfd0 to nonblock\n");
	    fcntl(sockfd0, F_SETFL, fcntl(sockfd0, F_GETFL, 0) | 
					O_NONBLOCK);
	
	    freeaddrinfo(servinfo);

	    //Enable "Listening" for Connections
	    printf("Enable listening\n");
	    listen(sockfd0, BACKLOG);

	    //Initialize Current Socket Address Info
	    printf("Initialize sockfd1 address\n");
	    char str_tcp1[4];
	    sprintf(str_tcp1, "%d", g_net_link[i].socket_tcp1);
	    printf("%d\n", g_net_link[i].socket_tcp1);
	    printf("%s\n", str_tcp1);
	    getaddrinfo(g_net_link[i].socket_domain1, 
			str_tcp1, &hints, &servinfo);

	    for(ps1 = servinfo; ps1 != NULL; ps1 = ps1->ai_next) {
		//Initialize Socket
		printf("Initialize sockfd1\n");
		sockfd1 = socket(ps1->ai_family, ps1->ai_socktype,
				ps1->ai_protocol);

		//Connect Socket
		printf("Connecting sockfd1\n");
		connect(sockfd1, ps1->ai_addr, ps1->ai_addrlen);

		break;
	    }

	   //Set Socket to Nonblocking
	   printf("Setting sockfd1 to nonblock\n");
	   fcntl(sockfd1, F_SETFL, fcntl(sockfd1, F_GETFL, 0) |
					O_NONBLOCK);
		
	    printf("inet_ntop start\n");
	    inet_ntop(ps1->ai_family, 
		      get_in_addr((struct sockaddr *) 
			ps1->ai_addr), s1, sizeof s1);
	    printf("inet_ntop end\n");

//	    printf("freeing servinfo\n");
//	    freeaddrinfo(servinfo);

	    printf("Handlers Start\n");
	    sa.sa_handler = sigchld_handler;
	    sigemptyset(&sa.sa_mask);
	    sa.sa_flags = SA_RESTART;
	    sigaction(SIGCHLD, &sa, NULL);
	    printf("Handlers End\n");
            
	    printf("Waiting for connections...\n");

//	    while(1) {
		sin_size = sizeof their_addr;

	    	//Wait for Connection
	    	int newsockfd = accept(sockfd0, 
					(struct sockaddr *)
					&their_addr, &sin_size);
		
		inet_ntop(their_addr.ss_family,
			  get_in_addr((struct sockaddr *)
					&their_addr), s0, 
					sizeof s0);

		printf("Got connection from %s\n", s0);
// 	    }
        }
    }

}

/*
 * Loads network configuration file and creates data structures
 * for nodes and links. 
 */
int load_net_data_file()
{
    FILE *fp;
    char fname[MAX_FILE_NAME];

    /* Open network configuration file */
    printf("Enter network data file: ");
    scanf("%s", fname);
    fp = fopen(fname, "r");
    if (fp == NULL) { 
        printf("net.c: File did not open\n"); 
        return(0);
    }

    int i;
    int node_num;
    char node_type;
    int node_id;

    /* 
     * Read node information from the file and
     * fill in data structure for nodes.
     * The data structure is an array g_net_node[ ]
     * and the size of the array is g_net_node_num.
     * Note that g_net_node[] and g_net_node_num are
     * private global variables.
     */
    fscanf(fp, "%d", &node_num);
    printf("Number of Nodes = %d: \n", node_num);
    g_net_node_num = node_num;

    if (node_num < 1) { 
        printf("net.c: No nodes\n");
        fclose(fp);
        return(0);
    }
    else { 
        g_net_node =(struct net_node*) malloc(sizeof(struct net_node)*node_num);
        for (i=0; i<node_num; i++) { 
            fscanf(fp, " %c ", &node_type);

            if (node_type == 'H') {
                fscanf(fp, " %d ", &node_id);
                g_net_node[i].type = HOST;
                g_net_node[i].id = node_id;
            }
            // Added this else if statement
            else if (node_type == 'S') {
                fscanf(fp, " %d ", &node_id);
                g_net_node[i].type = SWITCH;
                g_net_node[i].id = node_id;
            }
            else {
                printf(" net.c: Unidentified Node Type\n");
            }

            if (i != node_id) {
                printf(" net.c: Incorrect node id\n");
                fclose(fp);
                return(0);
            }
        }
    }
    /* 
     * Read link information from the file and
     * fill in data structure for links.
     * The data structure is an array g_net_link[ ]
     * and the size of the array is g_net_link_num.
     * Note that g_net_link[] and g_net_link_num are
     * private global variables.
     */

    int link_num;
    char link_type;
    int node0, node1;

    int snode;
    char domain0[MAX_DOMAIN_NAME]; 
    char domain1[MAX_DOMAIN_NAME];
    int tcp0, tcp1;

    fscanf(fp, " %d ", &link_num);
    printf("Number of links = %d\n", link_num);
    g_net_link_num = link_num;

    if (link_num < 1) { 
        printf("net.c: No links\n");
        fclose(fp);
        return(0);
    }
    else {
        g_net_link =(struct net_link*) malloc(sizeof(struct net_link)*link_num);
        for (i=0; i<link_num; i++) {
            fscanf(fp, " %c ", &link_type);
            if (link_type == 'P') {
                fscanf(fp," %d %d ", &node0, &node1);
                g_net_link[i].type = PIPE;
                g_net_link[i].pipe_node0 = node0;
                g_net_link[i].pipe_node1 = node1;
            }

	    else if(link_type == 'S') {
		fscanf(fp," %d %s %d %s %d ", &snode,
					   &domain0, &tcp0, 
					   &domain1, &tcp1);
		g_net_link[i].type = SOCKET;
		g_net_link[i].socket_node = snode;
		for(int j = 0; domain0[j] != '\0'; j++) {
			g_net_link[i].socket_domain0[j] = 
			domain0[j];
		}
//		printf("%s\n", domain0);
		g_net_link[i].socket_tcp0 = tcp0;
		for(int k = 0; domain1[k] != '\0'; k++) {
			g_net_link[i].socket_domain1[k] = 
			domain1[k];
		}
//		printf("%s\n", domain1);
		g_net_link[i].socket_tcp1 = tcp1;
	    }

            else {
                printf("   net.c: Unidentified link type\n");
            }

        }
    }

    /* Display the nodes and links of the network */
    printf("Nodes:\n");
    for (i=0; i<g_net_node_num; i++) {
        if (g_net_node[i].type == HOST) {
            printf("   Node %d HOST\n", g_net_node[i].id);
        }
        else if (g_net_node[i].type == SWITCH) {
            printf("   Node %d SWITCH\n", g_net_node[i].id);
        }
        else {
            printf(" Unknown Type\n");
        }
    }
    printf("Links:\n");
    for (i=0; i<g_net_link_num; i++) {
        if (g_net_link[i].type == PIPE) {
            printf("   Link (%d, %d) PIPE\n", 
                    g_net_link[i].pipe_node0, 
                    g_net_link[i].pipe_node1);
        }
        else if (g_net_link[i].type == SOCKET) {
//            printf("   Socket: to be constructed (net.c)\n");
	      printf("   Link (%d, %s, %d, %s, %d) SOCKET\n",
			g_net_link[i].socket_node,
			g_net_link[i].socket_domain0,
			g_net_link[i].socket_tcp0,
			g_net_link[i].socket_domain1,
			g_net_link[i].socket_tcp1);
        }
    }

    fclose(fp);
    return(1);
}



