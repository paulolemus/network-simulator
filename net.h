

/* net_init:
 * 1. first calls load_net_data_file:
 *    a. scans in and extracts data from config file
 *    b. Creates nodes and net links
 *    c. Displays the node and link data
 * 2. Calls create_node_list:
 *    a. Creates a linked list of net_node
 *    b. Sets net_node id, type, next;
 * 3. Calls create_port_list:
 *    a. For each g_net_link that is a pipe, it does:
 *       i. create net_port
 *      ii. creates 2 non-blocking pipe
 *     iii. adds pipe information to g_port_list
 * 4. Calls create_man_ports:
 *    a. Creates more pipes?
 */
int net_init();

struct man_port_at_man *net_get_man_ports_at_man_list();
struct man_port_at_host *net_get_host_port(int host_id);

struct net_node *net_get_node_list();
struct net_port *net_get_port_list(int host_id);


