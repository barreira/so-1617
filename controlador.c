#include <stdio.h>    // perror
#include <sys/stat.h> // mkfifo
#include <fcntl.h>    // open
#include <unistd.h>   // read
#include <string.h>   // strcmp, strtok
#include <stdlib.h>   // atoi

#include "node.h"

#define MAX_SIZE    1024
#define NUM_OPTIONS 32

LNode network;

void node(char** options)
{
    // node 1 windows 2 avg 10
    // options[0] = "node"
    // options[1] = "1"
    // options[2] = "windows"
    // options[3] = "2"
    // options[4] = "avg"
    // options[5] = "10"

    int i;
    char* cmd = "";

    for (i = 2; options[i] != NULL; i++) {
        strcat(cmd, options[i]);
        strcat(cmd, " ");
    }
 
    Node n = create_node(atoi(options[1]), cmd);

    add_node_to_network(network, n);
}

void connect(char** options)
{
    // connect 2 3 5
    // options[0] = "connect"
    // options[1] = "2"
    // options[2] = "3"
    // options[3] = "5"

    // 2->out acrescentar {3, 5}
    // 3->in acrescentar {2}
    // 5->in acrescentar {2}

    int i;
    char* connections = "";

    for (i = 1; options[i] != NULL; i++) {
        strcat(connections, options[i]);
        strcat(connections, " ");
    }

    add_connections(network, connections);
}

void disconnect(char** options)
{
    
}

void inject(char** options)
{
    
}

int interpretador(char* cmdline)
{
    char* options[NUM_OPTIONS];
    int i = 0;

    options[i] = strtok(cmdline, " ");

    while (options[i] != NULL) {
        options[++i] = strtok(NULL, " ");
    }

    if (strcmp(options[0], "node") == 0) {
        node(options);
    }

    else if (strcmp(options[0], "connect") == 0) {
        connect(options);
    }

    else if (strcmp(options[0], "disconnect") == 0) {
        disconnect(options);
    }

    else if (strcmp(options[0], "inject") == 0) {
        inject(options);
    }

    else {
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int n;
    int fd1;
    char* buffer;
    
    network = NULL;

    if (argc == 2) {
        // ler ficheiro de configuração (se existir)        
    }

    if (mkfifo("fifo", 0666) == -1) {
        perror("mkfifo");
    }

    if ((fd1 = open("fifo", O_RDONLY)) == -1) {
        perror("open");
        return 1;
    }

    while(1) {
        while((n = read(fd1, buffer, MAX_SIZE)) > 0 ) {
            interpretador(buffer);
        }
    }

    return 0;
}

/*
./controlador
./controlador ficheiro_configuracao.txt

./cliente node 1 window 2 avg 10
./cliente inject ...
*/
