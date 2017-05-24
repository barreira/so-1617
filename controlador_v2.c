#include <stdio.h>    // perror
#include <sys/stat.h> // mkfifo
#include <fcntl.h>    // open
#include <unistd.h>   // read
#include <string.h>   // strcmp, strtok
#include <stdlib.h>   // atoi
#include <limits.h>   // PIPE_BUF
#include <signal.h>   // sinais
#include <sys/wait.h> // wait

#define MAX_SIZE 1024

/* Variáveis globais */

int nodes[MAX_SIZE];    // array que indica se nó existe na rede
int nodespid[MAX_SIZE]; // array com os PIDs dos nós

typedef struct fanout {
    int pid;     // pid do fanout
    int* outs;   // array de IDs dos nós do output
    int numouts; // número de nós de output
} *Fanout;

Fanout connections[MAX_SIZE]; // índice indica in do fanout (ID do nó)
                              // tem PID do processo fanout (se estiver ativo)

void init_network()
{
    int i;

    for (i = 0; i < MAX_SIZE; i++) {
        nodes[i] = 0;
        connections[i] = NULL;
    }
}

/* Funções auxiliares */

Fanout create_fanout(int pid, int* outs, int numouts)
{
    int i;
    int* array;

    Fanout f = malloc(sizeof(struct fanout));
    array = malloc(sizeof(int) * numouts);

    for (i = 0; i < numouts; i++) {
        array[i] = outs[i];
    }

    f->pid = pid;
    f->outs = array;
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
    char in[15], out[15], buffer[MAX_SIZE], aux[5];

    signal(SIGUSR1, stop_fanout);

    sprintf(aux, "%d", input);
    sprintf(in, "./tmp/%sout", aux);

    fdi = open(in, O_RDONLY);
    
    if (fdi == -1) perror("open");

    // Abrir FIFOs de saída

    for (i = 0; i < numouts; i++) {
        sprintf(aux, "%d", outputs[i]);
        sprintf(out, "./tmp/%sin", aux);
	    fdos[i] = open(out, O_WRONLY);
	    if (fdos[i] == -1) perror("open");
    }
    
    // Escrever nos FIFOs de saída

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

    n = atoi(options[1]); // nó de entrada

    if (nodes[n] != -1) {
        printf("Já existe nó com ID %d\n", n);
        return 1;
    }

    /* Criar filho para correr o componente */

    nodespid[n] = fork();
    
    if (nodespid[n] == -1) perror("fork");
    
    if (nodespid[n] == 0) {
        
        /* Fazer FIFOs in e out e abri-los */

        char in[15], out[15];
        int fdi, fdo;

        sprintf(in, "./tmp/%sin", options[1]);
        if (!flag) { sprintf(out, "./tmp/%sout", options[1]); }

        mkfifo(in, 0666);
        if (!flag) { mkfifo(out, 0666); }
        
        fdi = open(in, O_RDONLY);
        if (!flag) { fdo = open(out, O_WRONLY); }
        
        /* Redirecionar para os FIFOs */

        dup2(fdi, 0);
        if (!flag) { dup2(fdo, 1); }
        
        /* Executar o componente */

        execvp(options[2], &options[3]);
    }
    
    /* Acrescentar nó à rede */   

    nodes[n] = 1;

    return 0;
}

int connect(char** options, int numoptions)
{
    int i, j = 0, n, pid, numouts;
    Fanout f;

    n = atoi(options[1]);

    numouts = numoptions - 2;

    int outs[numouts];

    if (connections[n] != NULL) {
        numouts += connections[n]->numouts;
        int outs[numouts];

        for (i = 0; i < connections[n]->numouts; i++) {
        	outs[++j] = connections[n]->outs[i];
   		}

        kill(connections[n]->pid, SIGUSR1);
        waitpid(connections[n]->pid, NULL, 0);
        connections[n] = NULL;
    }
    else {
        int outs[numouts];
    }
    
    for (i = 2; i < numoptions; i++) {
        outs[++j] = atoi(options[i]);
    }

    pid = fork();

    if (pid == -1) perror("fork");
    
    if (pid == 0) {     
        fanout(n, outs, numouts);
    }
    else {
        f = create_fanout(pid, outs, numouts);
        connections[n] = f;
    }

    return 0;
}

// e.g. disconnect 1 2
int disconnect(char** options)
{
    int a, b, exists = 0, numouts, i, j = 0, pid;
    Fanout f;

    a = atoi(options[1]);
    b = atoi(options[2]);

    // Verificar se existe alguma conexão para o IN (a) recebido

    if (connections[a] != NULL) {
        
        numouts = connections[a]->numouts;

        // Verificar se a conexão tem OUT (b) como saída

        for (i = 0; i < numouts; i++) {
            if (connections[a]->outs[i] == b) {
                exists = 1;
            }
        }

        if (!exists) { // A conexão não tem OUT (b) com saída
            return 1;
        }

        if (numouts == 1) {
            kill(connections[a]->pid, SIGUSR1);
            waitpid(connections[a]->pid, NULL, 0);
            connections[a] = NULL;         
            return 0;
        }
        else {
            int outs[numouts--];

            for (i = 0; i < connections[a]->numouts; i++) {
                if (connections[a]->outs[i] != b) {
                    outs[++j] = connections[a]->outs[i]; 
                }
            }

            kill(connections[a]->pid, SIGUSR1);
            waitpid(connections[a]->pid, NULL, 0);
            connections[a] = NULL;

            pid == fork();

            if (pid == -1) perror("fork");

            if (pid == 0) {     
                fanout(a, outs, numouts);
            }
            else {
                f = create_fanout(pid, outs, numouts);
                connections[a] = f;
            }
        }
    }
    else { // IN (a) não existe
        return 1;
    }

    return 0;
}

// e.g. inject 1 ...
int inject(char** options)
{
    int fd, pid;
    char in[15];

    sprintf(in, "./tmp/%sin", options[1]);

    fd = open(in, O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    pid = fork(); // é preciso guardar o pid nalgum lado?

    if (pid == 0) {
        dup2(fd, 1);
        execvp(options[2], &options[3]);
    }

    return 0;
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
        return node(options, 0);
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
    char buffer[MAX_SIZE];

    init_network();

    if (argc == 2) {
        // ler ficheiro de configuração
    }

    while (read(0, buffer, MAX_SIZE) > 0) {
        interpretador(buffer);
    }

    return 0;
}