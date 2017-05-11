#ifndef NODE_H
#define NODE_H

#define NUM_OPTIONS 32
#define MAX_INOUT 16

/* Estruturas */

typedef struct node {
    int id;
    char* cmd;
    int in[MAX_INOUT];  // GArray
    int out[MAX_INOUT]; // GSList* out;
} *Node;

/* API */

GSList* add_node_to_network(GSList* l, Node n);
void add_connections(GSList* l, char* connections);

Node create_node(int id, char* cmd);

#endif