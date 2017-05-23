#include <stdio.h>    // perror
#include <sys/stat.h> // mkfifo
#include <fcntl.h>    // open
#include <unistd.h>   // read
#include <string.h>   // strcmp, strtok
#include <stdlib.h>   // atoi
#include <limits.h>   // PIPE_BUF
#include <signal.h>   // sinais

#define MAX_SIZE 1024

/* Variáveis globais */

int nodes[MAX_SIZE];    // array que indica se nó existe na rede
int nodespid[MAX_SIZE]; // array com os PIDs dos nós

int numconnects;              // contador de processos de conexão
Fanout connections[MAX_SIZE]; // array com 

typedef struct fanout {
    int pid;            // pid do fanout
    int outs[MAX_SIZE]; // IDs dos nós do output
    int numouts;        // número de nós de output
} Fanout;

void init_network()
{
    int i;

    for (i = 0; i < MAX_SIZE; i++) {
        nodes[i] = -1;
        connections[i] = NULL;
    }
}

/* Funções auxiliares */

Fanout create_fanout(int pid, int* outs, int numouts)
{
    Fanout f = malloc(sizeof(struct fanout));
    
    f->pid = pid;
    f->outs = outs;
    f->numouts = numouts;

    return f;
}

int stopfan = 0;

void stop_fanout()
{
    stopfan = 1;
}

void fanout(int input, int outputs[], int numouts)
{
    int i, fdi, fdos[numouts], bytes;
    char in[15], out[15], buffer[MAX_SIZE];

    signal(SIGUSR1, stop_fanout);

    sprintf(in, "./tmp/%sout", input);

    fdi = open(in, O_RDONLY);
    
    for (i = 0; i < numouts; i++) {
        sprintf(out, "./tmp/%sin", outputs[i]);
	    fdos[i] = open(out, O_WRONLY);
    }

    while ((bytes = read(fdi, buffer, PIPE_BUF)) > 0 && !stopfan) {
        for (i = 0; i < numouts; i++) {
            write(fdos[i], buffer, bytes);
        }
    }
}

/* Comandos do controlador */

int node(char** options, int flag)
{
    int n;

    /* Verificar se o nó já existe na rede */

    n = atoi(options[1]);

    if (nodes[n] != -1) {
        printf("Já existe nó com ID %d\n", n);
        return 1;
    }

    /* Criar filho para correr o componente */

    nodespid[n] = fork();
    
    if (nodespid[n] == 0) {
        
        /* Fazer FIFOs in e out e abri-los */

        char in[15], out[15];
        int fdi, fdo;

        sprintf(in, "./tmp/%sin", n);
        if (!flag) {
            sprintf(out, "./tmp/%sout", n);
        }

        mkfifo(in, 0666);
        if (!flag) {
            mkfifo(out, 0666);
        }
        
        fdi = open(in, O_RDONLY);
        if (!flag) {
            fdo = open(out, O_WRONLY);
        }
        
        /* Redirecionar para os FIFOs */

        dup2(fdi, 0);
        if (!flag) {
            dup2(fdo, 1);
        }
        
        /* Executar o componente */

        execvp(options[2], &options[3]);
    }
    
    /* Acrescentar nó à rede */   

    nodes[n] = 1;

    return 0;
}

int connect(char** options, int numoptions)
{
    int i, j = 0;
    int numouts = numoptions - 2;
    int n, pid, numouts, outs[numouts];
    Fanout f;

    n = atoi(options[1]);

    // Verifica-se se já existia conexão a partir daquele nó
    // Se sim, mata conexão. Se não, aumenta o número total de conexões.

    if (connections[n] != NULL) {
        kill(connections[n]->pid, SIGUSR1);
    }
    else {
        numconnects++;
    }

    // Constroi o array de nós do output

    for (i = 2; i < numoptions; i++) {
        outs[++j] = atoi(options[i]);
    }

    // Cria processo da conexão e põe-no a fazer fanout

    pid = fork();

    if (pid == 0) {
        fanout(n, outs, numouts);
    }

    // Cria o fanout (struct) e adiciona-o à lista das conexões

    f = create_fanout(pid, outs, numouts);
    connections[n] = f;

    return 0;
}

int disconnect(char** options)
{

}

int inject(char** options)
{
    
}

/* Interpretador de comandos */

int interpretador(char* cmdline)
{
    int i = 0;
    char* options[MAX_SIZE];

    options[i] = strtok(cmdline, " ");

    while (options[i] != NULL) {
        options[++i] = strtok(NULL, " ");
    }

    if (strcmp(options[0], "node") == 0) {
        return node(options);
    }

    else if (strcmp(options[0], "connect") == 0) {
        return connect(options, i);
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
    init_network();

    if (argc == 2) {
        // ler ficheiro de configuração
    }

    while (read(0, buffer, MAX_SIZE) > 0) {
        interpretador(buffer);
    }

    return 0;
}