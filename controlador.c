#include <stdio.h>    // perror
#include <sys/stat.h> // mkfifo
#include <fcntl.h>    // open
#include <unistd.h>   // read
#include <string.h>   // strcmp, strtok
#include <stdlib.h>   // atoi
#include <limits.h> //PIPE_BUF
#include <signal.h> //sinais
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
*/
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



int main(int argc, char* argv[]) {
    int n;
    int fd1;
    char* buffer;
    
    if (argc == 2) {
        // ler ficheiro de configuração (se existir) e executar linha a linha

        //avisar que carregou com sucesso ?
    }
    /*
    //loop leitura de comandos
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

    int node(char *num, char *cmd[]) {
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

	//inicializações 
	int fnum = 0; //contador de fanout(s)
	int fpid[MAX_SIZE]; //pids fanout
	int fstop[MAX_SIZE]; //fanout terminar - tem de ser global
	int fin[MAX_SIZE]; //fanout fnum in
	for(i=0;i<MAX_SIZE;i++) { fstop[i] = 0 ; fin[i] = 0; } // não há nodo 0  zerar runs e fin

	//função signal para a fanout
	void stopfan(int n) { fstop[n] = 1; } //fanout n para parar na próxima iteração

	void connect(char nodo, char out[]) {
		fnum++; //aumentar fnum
		int a = atoi(nodo);
		for(i=0;i<MAX_SIZE;i++) { //verificar nos fanouts se algum está a ler do nodo ; se sim: matar fanout e criar fanout com novos valores
			if(fin[i] == a) { kill(fpid[i],SIGUSR1); break; } 
		}
		//fork, correr fanout, guardar pid fannout para mandar signal
		fpid[fnum] = fork();
		if(fpid[fnum] == 0) {
		fin[fnum] = a; //fannout num fnum recebe input nodo a
		fanout(nodo, out,fnum); //criar fannout n para usar no fstop
		}
	}
// ############# CONNECT END ######################################

// #### FANOUT ###############################
	void fanout(char input, char outputs[], int n) {// in = fifo in , out = fifo(s) out, n = fnum number

	    signal(SIGUSR1, stopfan); //pára o fanout na próxima iteração, inicio da main ?
	    char in[15],out[15];
		int bytes,i,osize=1,inputs;
	    char buffer[MAX_SIZE];
	    //input
	    sprintf(in,"./tmp/%sin",input); // Nin
	    inputs = open(in,O_RDONLY); //abrir fifo entrada leitura
	    //descobrir número outputs
	    for(i=0;i<osize;i++) { if(outputs[i] != '\0') osize++ ; }
	    //abrir fifos outputs para escrita
	    int prints[osize];
	    for(i=0;i<osize;i++) { 
	    	sprintf(out,"./tmp/%sout",outputs[i]); //Nout
	    	prints[i] = open(out,O_WRONLY); //verificar se abriu ?
	    }
	    //loop leitura e escrita em todos os outputs
	    while ((bytes = read(inputs, buffer, PIPE_BUF)) > 0 && (fstop[n] == 0)) {
	    	//readline

	        // faz os writes em todos os outputs (prints[])
	        for (i = 0; i < osize; i++) {
	            write(prints[i], buffer, bytes);
	        }
	    }
	    //fechar fifos escrita/leitura ao receber signal para fstop ? ou não é preciso ?
	    //if (fstop[n] == 1) for ... close(...)
	}
//##################FANOUT END ##########################


    //testar criar nodos
    char *test[3] = {"./const","./const","10"};
    char *test2[3] = {"./const","./const","20"};
    node("1",test);
    node("2",test2);
    node("1",test2); //erro ao criar
    printf("fiz os creates\n");
    return 0;
}

/*
./controlador
./controlador ficheiro_configuracao.txt

./cliente node 1 window 2 avg 10
./cliente inject ...
*/
