#include <stdio.h>    // perror
#include <sys/stat.h> // mkfifo
#include <fcntl.h>    // open
#include <unistd.h>   // read
#include <string.h>   // strcmp, strtok
#include <stdlib.h>   // atoi

#define MAX_SIZE 1024

pid_t* network = NULL;
int numnodes = 1;
int nodecap = 1;

pid_t* add_node_to_network(pid_t pid)
{
    if (network == NULL) {
        network = malloc(sizeof(pid_t));
        network[0] = pid;
    }

    else if (numnodes == nodecap) {
        network = realloc(network, sizeof(pid_t) * nodecap * 2);
        nodecap *= 2;
        network[numnodes] = pid;
        numnodes++;
    }

    return network;
}

int exec_component(char** cmd)
{
    if (strcmp(cmd[0], "const") == 0) {
        const(&cmd[1]); // falta corrigir isto
    }

    else if (strcmp(cmd[0], "filter") == 0) {
        filter(&cmd[1]); // falta corrigir isto
    }

    else if (strcmp(cmd[0], "window") == 0) {
        window(&cmd[1]); // falta corrigir isto
    }

    else if (strcmp(cmd[0], "spawn") == 0) {
        spawn(&cmd[1]); // falta corrigir isto
    }

    else { /* Componente não existe */
        return 1;
    }

    return 0;    
}


int node(char** options) // e.g node 1 window 2 avg 10
{
    int wr[2], rd[2];
    pid_t pid, first, second, third;
    ssize_t bytes;
    char buff_rd[MAX_SIZE], buff_wr[MAX_SIZE];

    pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        pipe(wr);
        pipe(rd);

        first = fork();

        if (first == 0) { // leitor
            close(wr[0]);
            close(rd[0]);
            close(rd[1]);

            while ((bytes = read(0, buff_rd, MAX_SIZE)) > 0) { // tem de ser readln
                write(wr[1], buff_rd, bytes);
            }

            _exit(0);
        }

        second = fork();

        if (second == 0) { // filtro
            dup2(wr[0], 0);
            dup2(rd[1], 1);

            close(wr[1]);
            close(rd[0]);

            exec_component(&options[2]);

            _exit(0);
        }

        third = fork();

        if (third == 0) { // escritor
            close(rd[1]);
            close(wr[0]);
            close(wr[1]);

            while ((bytes = read(rd[0], buff_wr, MAX_SIZE)) > 0) { // tem de ser readln
                write(1, buff_wr, bytes);
            }

            _exit(0);
        }

        close(wr[0]);
        close(wr[1]);
        close(rd[0]);
        close(rd[1]);
    }
    else {
        network = add_node_to_network(pid);
    }

    return 0;
}

int connect(char** options)
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

    //add_connections(network, connections);
}

int disconnect(char** options)
{
    // disconnect 2 3
    // options[0] = "disconnect"
    // options[1] = "2"
    // options[2] = "3"

    int i;
    char* connections = "";

    for (i = 1; options[i] != NULL; i++) {
        strcat(connections, options[i]);
        strcat(connections, " ");
    }

    //close_connections(network, connections);
}

int inject(char** options)
{
    
}

int interpretador(char* cmdline)
{
    char* options[MAX_SIZE];
    int i = 0, error = 0;

    options[i] = strtok(cmdline, " ");

    while (options[i] != NULL) {
        options[++i] = strtok(NULL, " ");
    }

    if (strcmp(options[0], "node") == 0) {
        return node(options);
    }

    else if (strcmp(options[0], "connect") == 0) {
        return connect(options);
    }

    else if (strcmp(options[0], "disconnect") == 0) {
        return disconnect(options);
    }

    else if (strcmp(options[0], "inject") == 0) {
        return inject(options);
    }

    else { /* Comando não existe */
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
