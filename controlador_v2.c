#include ...

#define MAX_SIZE 1024

/* Variáveis globais */

int nodes[MAX_SIZE];
int nodespid[MAX_SIZE];

int numconnects;
Fanout connections[MAX_SIZE]; // nó do input corresponderia ao indice deste array

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

// void fanout(int n, Fanout f) ?
void fanout(int input, int outputs[], int numouts)
{
    int i, fdi, fdos[numouts], bytes, stop = 0;
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

// e.g. connect 2 4 7
int connect(char** options, int numoptions)
{
    int i, j = 0;
    int numouts = numoptions - 2;
    int n, pid, numouts, outs[numouts];
    Fanout f;

    n = atoi(options[1]);

    if (connections[n] != NULL) {
        kill(connections[n]->pid, SIGUSR1);
    }
    else {
        numconnects++;
    }

    for (i = 2; i < numoptions; i++) {
        outs[++j] = atoi(options[i]);
    }

    pid = fork();

    if (pid == 0) {
        fanout(n, outs, numouts);
    }

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