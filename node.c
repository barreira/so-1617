#include <stdlib.h> // malloc, null
#include <string.h> // strdup, strtok
#include <glib.h>   // linked list

#include "node.h"

/* FREE */

void freeNode(Node n)
{
    free(n->cmd);
    free(n);
    n = NULL;
}

/* FUNÇÕES AUXILIARES */

GSList* add_node_to_network(GSList* l, Node n)
{  
    l = g_slist_append(l, n);
    return l;
}

GSList* get_node(GSList* l, int id)
{
    Node n = create_node(id, NULL);

    GSList* res = g_slist_find(l, n);

    freeNode(n);

    return res;
}

/* CLOSE CONNECTIONS */

void close_connections(GSList* l, char* connections)
{

}

/* ADD CONNECTIONS */

GSList* add_in(GSList* n, int id)
{
    int i;

    for (i = 0; ((Node) n)->in[i] != 0; i++);

    ((Node) n)->in[i] = id;

    return n;
}

GSList* add_out(GSList* n, int id)
{
    int i;

    for (i = 0; ((Node) n)->out[i] != 0; i++);

    ((Node) n)->out[i] = id;

    return n;
}

void add_connections(GSList* l, char* connections)
{
    // connections = "2 3 5 "

    int i;
    char* nodes[NUM_OPTIONS];

    nodes[i] = strtok(connections, " ");

    while (nodes[i] != NULL) {
        nodes[++i] = strtok(NULL, " ");
    }

    // nodes = { 2, 3, 5 }

    GSList* n1 = get_node(l, atoi(nodes[0]));
    GSList* tmp;

    for (i = 1; nodes[i] != NULL; i++) {
        n1 = add_out(n1, atoi(nodes[i]));
        tmp = get_node(l, atoi(nodes[i]));
        tmp = add_in(tmp, atoi(nodes[0]));
    }
}

/* CREATE NODE */

Node create_node(int id, char* cmd)
{
    int i;
    Node n = malloc(sizeof(struct node));

    n->id = id;
    n->cmd = strdup(cmd);

    for (i = 0; i < MAX_INOUT; i++) {
        n->in[i] = 0;
        n->out[i] = 0;
    }

    return n;
}