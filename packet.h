/* Definitions and prototypes for the link (link.c)
 */


// receive packet on port
int packet_recv(struct net_port *port, struct packet *p);

// send packet on port
void packet_send(struct net_port *port, struct packet *p);

// special receive packet on port for switch
int switch_packet_recv(struct net_port* port, struct packet* p, struct sockaddr_storage** addr, socklen_t* addr_size);
