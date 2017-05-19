#include <stdio.h>    // perror
#include <sys/stat.h> // mkfifo
#include <fcntl.h>    // open
#include <unistd.h>   // read
#include <string.h>   // strcmp, strtok
#include <stdlib.h>   // atoi
#include <limits.h> //PIPE_BUF
//#include <glib.h>     // linked list

#define MAX_SIZE 1024

//GSList* network = NULL;


void fanin(int in[], int n, int out) {}


/*
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

    else { // Componente não existe 
        return 1;
    }

    return 0;    
}
*/
// node 1 windows 2 avg 10
// options[0] = "node"
// options[1] = "1"
// options[2] = "windows"
// options[3] = "2"
// options[4] = "avg"
// options[5] = "10"

/*
int node(char** options) // e.g node 1 window 2 avg 10
{   
    int i, nb1, nb2, nb3;
    int pid, first, second, third;
    int wr[2], rd[2];
    char buff_rd[MAX_SIZE], buff_wr[MAX_SIZE], buff_io[MAX_SIZE];
    char* component = "", pipe_name = "pipes/io/";

    for (i = 2; options[i] != NULL; i++) {
        strcat(component, options[i]);
        strcat(component, " ");
    }

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

            while ((nb1 = read(0, buff_rd, MAX_SIZE)) > 0) { // tem de ser readln
                write(wr[1], buff_rd, nb1);
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

            while ((nb2 = read(rd[0], buff_wr, MAX_SIZE)) > 0) { // tem de ser readln
                write(n->out, buff_wr, nb2);
            }

            _exit(0);
        }

        close(wr[0]);
        close(wr[1]);
        close(rd[0]);
        close(rd[1]);

        // Nó vai estar sempre a ler do fifo das mudanças de IO
        strcat(pipe_name, itoa(getpid()));
        fifo_io = open(pipe_name, O_RDONLY);

        while ((nb3 = read(fifo_io, buff_io, MAX_SIZE)) > 0) {
            change_io();
        }
    }
    else {
        // Cria-se uma nova struct Node para guardar as informações do nó criado
        Node n = create_node(atoi(options[0]), pid, component);
        // Adiciona-se o novo nó à lista ligada dos nós (do controlador)
        network = g_slist_append(network, n);

        // Cria-se o fifo que servirá para o controlador comunicar aos nós as mudanças de IO (aquando do connect)
        strcat(pipe_name, itoa(pid));
        mkfifo(pipe_name, 0666);
    }

    return 0;
}
*/


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



    // comunicar mudanças aos nós

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

int inject(char** options) {  }
/*
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

    else { // Comando não existe 
        return 1;
    }

    return 0;
}
*/



int main(int argc, char* argv[])
{
    int n;
    int fd1;
    char* buffer;
    
    //network = NULL;

    if (argc == 2) {
        // ler ficheiro de configuração (se existir) e executar linha a linha
    }
    /*
    if (mkfifo("fifo", 0666) == -1) {
        perror("mkfifo");
    }

    if ((fd1 = open("fifo", O_RDONLY)) == -1) {
        perror("open");
        return 1;
    }

    while(1) {
        while((n = read(0, buffer, MAX_SIZE)) > 0 ) {
            interpretador(buffer);
        }
    }
    */

// ########### criar nodo ##########################################
    int nodos[MAX_SIZE]; //num max de nodos, inicializar todos os valores a -1
    int nodospid[MAX_SIZE]; //guardar pid nodo (para eventualmente terminar no futuro)
    int i;
    for (i = 0 ; i < 100 ; i++) { nodos[i] = 0; }

    int create_node(char *num, char *cmd[]) {
        //verificar se já não existe esse nodo
        int a = atoi(num);
        if (nodos[a] != 0) { printf("Já existe esse nodo: %d\n",a); return 1; } //tratar o erro?
        // fork para correr o filtro
        nodospid[a] = fork();
        if(nodospid[a] == 0) {
            //fazer fifo in e out
            char in[15],out[15];
            int fdi,fdo;
            sprintf(in,"./tmp/%sin",num); // Nin
            sprintf(out,"./tmp/%sout",num); //Nout
            printf("valor de in: %s e valor de out: %s\n",in, out);
            if(mkfifo(in, 0666) == -1) { perror("fifo in falhou"); } 
            if(mkfifo(out, 0666) == -1) { perror("fifo out falhou"); } 
            fdi = open(in,O_RDONLY); //fifo leitura
            fdo = open(out,O_WRONLY); //fifo escrita
            //rederecionar fifos
            dup2(fdi,0); //stdin para fifo in
            dup2(fdo,1); //stdout para fifo out
            //executar filtro e terminou por aqui
            //execvp(cmd[0],cmd);
            execlp("ls","ls","-la",NULL);
        }
        //criado com sucesso, actualizar valor nodo
        nodos[a] = 1;
        return 0;
}
// ################## NODE END ############################################

// ########### CONNECT ##############################################
//connect nodoX nodo(s)
//connect 1 2
//connect 1 2 3 4

//função signal para a fanout
void stopfan(int n) { fstop[n] = 1; } //fanout n para parar na próxima iteração

//inicializações 
int fnum = 0; //contador de fanout(s)
int fpid[MAX_SIZE]; //pids fanout
int fstop[MAX_SIZE]; //fanout terminar - tem de ser global
int fin[MAX_SIZE]; //fanout fnum in
for(i=0;i<MAX_SIZE;i++) { fstop[i] = 0 ; fin[i] = 0; //não há nodo 0 } //zerar runs e fin


void connect(char nodo, char out[]) {
	fnum++; //aumentar fnum
	a = atoi(nodo);
	for(i=0;i<MAX_SIZE;i++) { //verificar nos fanouts se algum está a ler do nodo ; se sim: matar fanout e criar fanout com novos valores
		if(fin[i] == a) { stopfan(a); break; } 
	}
	//fork, correr fanout, guardar pid fannout para mandar signal
	fork();
	actualizar fin[fnum] = a;
	fanout(a, out[],fnum); //criar fannout n para usar no fstop
}





// ############# CONNECT END ######################################

// #### FANOUT ###############################
/*
void fanout(char in, char out[], int n) // in = fifo in , out = fifo(s) out, n = fnum number
{
    signal(SIGUSR1, stopfan);
    char in[15],out[15];
    int fdi,fdo;
    sprintf(in,"./tmp/%sin",num); // Nin
    sprintf(out,"./tmp/%sout",num); //Nout
    int bytes, i;
    char buffer[MAX_SIZE];
    reader = open()

    while ((bytes = read(in, buffer, MAX_SIZE)) > 0 && running) {
        // executa comando
        for (i = 0; i < n; i++) {
            write(out[i], buffer, bytes);
        }
    }
}

*/

//##################FANOUT END ##########################


    //testar criar nodos
    char *test[3] = {"./const","./const","10"};
    char *test2[3] = {"./const","./const","20"};
    create_node("1",test);
    //create_node("2",test2);
    create_node("1",test2); //erro ao criar
    printf("fiz os creates\n");
    return 0;
}

/*
./controlador
./controlador ficheiro_configuracao.txt

./cliente node 1 window 2 avg 10
./cliente inject ...
*/
