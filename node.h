#ifndef NODE_H
#define NODE_H

typdef struct node {
    int id;
    int pid, pid_rd, pid_wr, pid_exec;
    int in, out;
    char* component;
} *Node;

Node create_node(int id, int pid, char* component);

#endif
