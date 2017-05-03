#ifndef NODE_H
#define NODE_H

/* Estruturas */

typedef struct node {
    int id;
    char* cmd;
    int* in;
    int* out;
} *Node;

typedef struct lnode {
    Node n;
    struct lnode *next;
} *LNode;

/* API */

void add_node_to_network(LNode l, Node n);
void add_connections(LNode l, char* nodes);

Node create_node(int id, char* cmd);
Node add_in(Node n, char* ins);

#endif