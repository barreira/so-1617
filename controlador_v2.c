#include ...

#define MAX_SIZE 1024

/* Variáveis globais */

int nodes[MAX_SIZE];
int nodespid[MAX_SIZE];

void init_network()
{
    int i;

    for (i = 0; i < MAX_SIZE; i++) {
        nodes[i] = -1;
    }
}

/* Funções auxiliares */

void fanout()
{

}

/* Comandos do controlador */

// e.g. node 1 window 2 avg 10
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

int connect(char** options)
{

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
    int fd;

    if (argc == 2) {
        // ler ficheiro de configuração
    }

    mkfifo("fifo", 0666);

    fd = open("fifo", O_RDONLY);

    init_network();

    while (1) {
        while (read(fd, buffer, MAX_SIZE) > 0) {
            interpretador(buffer);
        }
    }

    return 0;
}