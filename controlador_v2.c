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

Fanout connections[MAX_SIZE]; // array com ID in fannout, activo se != -1/NULL

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

    fdi = open(in, O_RDONLY); if(fdin < 1) perror("Falhou o open do fifo do input do fanout");
    //abrir fifos de saida
    for (i = 0; i < numouts; i++) {
        sprintf(out, "./tmp/%sin", outputs[i]);
	    fdos[i] = open(out, O_WRONLY);
	    if(fdos[i] < 1) perror("Falhou o open do fifo para output do fanout");
    }
    //escrita nos nós
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

    n = atoi(options[1]); //nó de entrada

    if (nodes[n] != -1) {
        printf("Já existe nó com ID %d\n", n);
        return 1;
    }

    /* Criar filho para correr o componente */

    nodespid[n] = fork(); if (fork() < 0) perror("Falhou fork() ai criador o node");
    
    if (nodespid[n] == 0) {
        
        /* Fazer FIFOs in e out e abri-los */

        char in[15], out[15];
        int fdi, fdo;

        sprintf(in, "./tmp/%sin", n);
        if (!flag) { sprintf(out, "./tmp/%sout", n); }

        mkfifo(in, 0666);
        if (!flag) { mkfifo(out, 0666);  }
        
        fdi = open(in, O_RDONLY);
        if (!flag) { fdo = open(out, O_WRONLY); }
        
        /* Redirecionar para os FIFOs */

        dup2(fdi, 0);
        if (!flag) { dup2(fdo, 1);}
        
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
    // Se sim, mata conexão copia saidas e faz um novo fannout com a informação adicional
    /* Quando existe um fannout a ler daquele input */
    if (connections[n] != NULL) { //já existe um fannout a ler desse nodo
    	int tmpouts[20];
    	int tmpnouts;
    	tmpouts = connectons[n]->outs;
    	tmpnouts = connectons[n]->numouts;
        kill(connections[n]->pid, SIGUSR1);
        waitpid(connections[n]->pid,NULL,0); //esperar que o fannout termine e executar o novo com as novas saidas
        //criar o primeiro outs[]
   		for (i = 2; i < numoptions; i++) { //aqui não deveria ser numoptions-2 ? senão o ciclo pára logo, certo? com 2 outs por exemplo. ou o numouts? :)
        	outs[++j] = atoi(options[i]);
   		 }
   		//juntar com os novos
   		j=0;
        for(i=tmpouts;i<numouts+tmpnouts;i++) {
        	tmpouts[i] = outs[++j];
        }
        int total = numouts+tmpnouts;
    	pid = fork();
    	if(pid< 0) perror("Falhou a criação do fork no fannout");
    	if (pid == 0) {
    	// Cria o fanout (struct) e adiciona-o à lista das conexões
        f = create_fanout(pid, outs, numouts);
    	connections[n] = f;
    	 //void fanout(int input, int outputs[], int numouts)
    	fanout(n, outs, numouts);
    }
 
    }
    /* quando não existe nenhum fannout a ler daquele node */
    else { 
    // Constroi o array de nós do output
    for (i = 2; i < numoptions; i++) { //aqui não deveria ser numoptions-2 ? senão o ciclo pára logo, certo? com 2 outs por exemplo. ou o numouts? :)
        outs[++j] = atoi(options[i]);
    }

    // Cria processo da conexão e põe-no a fazer fanout
    pid = fork();
    if(pid< 0) perror("Falhou a criação do fork no fannout");
    if (pid == 0) {
    	// Cria o fanout (struct) e adiciona-o à lista das conexões
        f = create_fanout(pid, outs, numouts);
    	connections[n] = f;
    	//void create_fanout(int input, int outputs[], int numouts)
    	fanout(n, outs, numouts);
    }

    }
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