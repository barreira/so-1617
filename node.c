#include "node.h"

Node create_node(int id, int pid, char* component)
{
    Node n = malloc(sizeof(struct node));

    n->id = id;
    n->pid = pid;
    n->component = strdup(component);

    return n;
}