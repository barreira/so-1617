#include <stdlib.h> // malloc, null
#include <string.h> // strdup, strtok

#include "node.h"

#define NUM_OPTIONS 32

LNode create_network(Node n)
{
    LNode l = malloc(sizeof(struct lnode));

    l->n = n;
    l->next = NULL;

    return l;
}

void add_node_to_network(LNode l, Node n)
{
    if (l = NULL) {
        l = create_network(n);
    }

    LNode tmp = l;

    while (tmp->next != NULL) {
        tmp = tmp->next;
    }

    tmp->n = n;
}

void add_connections(LNode l, char* connections)
{
    // connections = "2 3 5 "

    int i;
    char* nodes[NUM_OPTIONS];

    nodes[i] = strtok(connections, " ");

    while (nodes[i] != NULL) {
        nodes[++i] = strtok(NULL, " ");
    }

    // nodes = { 2, 3, 5 }

    Node n1 = get_node(l, nodes[0]);
    Node tmp;

    for (i = 1; nodes[i] != NULL; i++) {
        add_out(n1, nodes[i]); // DEFINIR
        tmp = get_node(l, nodes[i]); // DEFINIR
        add_in(tmp, nodes[0]); // DEFINIR
    }
}

Node create_node(int id, char* cmd)
{
    Node n = malloc(sizeof(struct node));

    n->id = id;
    n->cmd = strdup(cmd);
    n->in = NULL;
    n->out = NULL;

    return n;
}