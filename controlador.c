#include <stdio.h>    // perror
#include <sys/stat.h> // mkfifo
#include <fcntl.h>    // open
#include <unistd.h>   // read
#include <string.h>   // strcmp, strtok
#include <stdlib.h>   // atoi
#include <limits.h> //PIPE_BUF
#include <signal.h> //sinais

#define MAX_SIZE 2048
int stop;

//int FANTOKILL; //global para matar fanout
/*
int exec_component(char** cmd)
{
    if (strcmp(cmd[0], "const") == 0) { const(&cmd[1]);  } // falta corrigir isto    
    else if (strcmp(cmd[0], "filter") == 0) { filter(&cmd[1]); } // falta corrigir isto
    else if (strcmp(cmd[0], "window") == 0) {   window(&cmd[1]); } // falta corrigir isto
    else if (strcmp(cmd[0], "spawn") == 0) {   spawn(&cmd[1]);  } // falta corrigir isto
    else { return 1 } // Componente não existe 
    return 0;    
}

// node 1 windows 2 avg 10
// options[0] = "node"
// options[1] = "1"
// options[2] = "windows"
// options[3] = "2"
// options[4] = "avg"
// options[5] = "10"
*/
//int inject(char** options) {  }
/*
int interpretador(char* cmdline)
{
    char* options[MAX_SIZE];
    int i = 0, error = 0;

    options[i] = strtok(cmdline, " ");

    while (options[i] != NULL) { options[++i] = strtok(NULL, " "); }

    if (strcmp(options[0], "node") == 0) { return node(options); }
    else if (strcmp(options[0], "connect") == 0) { return connect(options); }
	else if (strcmp(options[0], "disconnect") == 0) { return disconnect(options); }
	else if (strcmp(options[0], "inject") == 0) { return inject(options); }
	else {  return 1; } // Comando não existe

    return 0;
}
*/



int main(int argc, char* argv[]) {
    int n;
    int fd1;
    char* buffer[PIPE_BUF];
    
    if (argc == 2) {
        // ler ficheiro de configuração (se existir) e executar linha a linha

        //avisar que carregou com sucesso ?
    }
    /*
    //loop leitura de comandos
    while(1) {
   		while((n = read(0,buffer,PIPE_BUF)) >= 0) {  
      		if(n!=0) { //não parar com EOF
      			}
            interpretador(buffer);
        }
    }
    */

// ########### criar nodo ##########################################
    int nodos[MAX_SIZE]; //num max de nodos, inicializar todos os valores a -1
    int nodospid[MAX_SIZE]; //guardar pid nodo (para eventualmente terminar no futuro)
    int i;
    for (i = 0 ; i < 100 ; i++) { nodos[i] = 0; }

    // (numero nodo, comando a executar, output descartar)
    int node(char *num, char *cmd[], int option) { // option = 0 um dos filtros, 1 = descartar output
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
            if(!option) sprintf(out,"./tmp/%sout",num); //Nout
            //printf("valor de in: %s e valor de out: %s\n",in, out);
            if(mkfifo(in, 0666) == -1) { perror("fifo in falhou"); } 
            if(!option) { if(mkfifo(out, 0666) == -1) { perror("fifo out falhou"); } }
            fdi = open(in,O_RDONLY); //fifo leitura
            if(!option) fdo = open(out,O_WRONLY); //fifo escrita
            //rederecionar fifos
            dup2(fdi,0); //stdin para fifo in
            if(!option) dup2(fdo,1); //stdout para fifo out
            //executar filtro e terminou por aqui
            execvp(cmd[0],cmd);
            //execlp("ls","ls","-la",NULL);
        }
        //criado com sucesso, actualizar valor nodo
        nodos[a] = 1;
        return 0;
	}
// ################## NODE END ############################################

	//inicializações fanout
	int fnum = 0; //contador de fanout(s)
	int fpid[MAX_SIZE]; //pids fanout
	//int fstop[MAX_SIZE]; //fanout terminar - tem de ser global
	int fin[MAX_SIZE]; //fanout fnum in
	for(i=0;i<MAX_SIZE;i++) { fin[i] = 0; } // não há nodo 0  zerar runs e fin //fstop[i] = 0 ; 
	int fnouts[MAX_SIZE]; //numero de outs por fan - fnum 
	char fouts[MAX_SIZE][10]; //os outs daquele fan - fnum, Max de 10 outs.

//função signal para a fanout
void stopfan() { stop = 1; } //fanout n para parar na próxima iteração


// #### FANOUT ###############################

	void fanout(char *input, char *outputs[], int n, int outs) {// in = fifo in , out = fifo(s) out, n = fnum number, outs = numero de saidas

	    signal(SIGUSR1, stopfan); //pára o fanout na próxima iteração
	    char inp[15],outp[15];
		int bytes,i,fdin,prints[outs];
	    char buffer[MAX_SIZE];
	    int stop = 0;
	    //input
	    sprintf(inp,"./tmp/%sout",input); // output do nodo input
	    //write(1,"Vou abrir input:\n",16);
	    fdin = open("./tmp/1out",O_RDONLY);//abrir fifo entrada leitura
	    if(fdin < 1) perror("Falhou o open no fanout");
	    //abrir fifos outputs para escrita
	    for(i=0;i<outs;i++) { 
	    	sprintf(outp,"./tmp/%sin",outputs[i]); //escrever nos inputs dos nos
	    	prints[i] = open(outp,O_WRONLY); //verificar se abriu 
	    	if(prints[i] < 1) perror("Falhou o open no fanout");
	    	}
	    //loop leitura e escrita em todos os outputs
	    while ((bytes = read(fdin, buffer, PIPE_BUF)) > 0 && (stop == 0)) {
	    	//readline
	        // faz os writes em todos os outputs (prints[])
	        for (i = 0; i < outs; i++) {
	            write(prints[i], buffer, bytes);
	        }
	    }
	    //fechar fifos escrita/leitura ao receber signal para fstop
	    //if (fstop[n] == 1) for ... close(...)
	    //fin[fnum] == 0; //tirar do input
	    // (...)
	    //exit() - apanhar o exit para executar o novo fanout
	}
//##################FANOUT END ##########################



// ########### CONNECT ##############################################
//connect nodoX nodo(s)
//connect 1 2
//connect 1 2 3 4

void connect(char *nodo, char *out[], int nouts) {
		fnum++; //aumentar fnum
		int a = atoi(nodo);
		//verificar nos fanouts se algum está a ler do nodo ; se sim: matar fanout e criar fanout com novos valores
		//antes de matar, verificar os nouts e outputs e mante-los. aumentar nouts, out[] e etc. TODO
		for(i=0;i<MAX_SIZE-1;i++) { 
			if(fin[i] == a) { kill(fpid[i],SIGUSR1); break; } 
		}


		//fork, correr fanout, guardar pid fannout para mandar signal
		fpid[fnum] = fork();
		if(fpid[fnum] == 0) {
			fanout(nodo, out, fnum, nouts); //criar fannout, fnum para usar no fstop	
		}
		fin[fnum] = a; //fannout num fnum recebe input nodo a
		fnouts[fnum] = nouts; //guardar numero de outs
		for(i=0;i<nouts;i++) {
				fouts[fnum][i] = out[i];
			}
		//char *lixo[100];
		//sprintf(lixo,"fannout com fpid %d, fnum= %d e fin[fnum]= %d a= %d fin[1] = %d\n", fpid[fnum],fnum, fin[fnum],a, fin[1]);
		//write(1,lixo,strlen(lixo));
	}
// ############# CONNECT END ######################################

// ########### DISCONNECT ##############################################
// disconnect <id1> <id2>
void disconnect(char *nodo, char *remover){
	//verificar fannout que tem aquele input, ver os outputs totais, se >1, manter e remover o especificado, caso contrário enviar signal para matar aquele fanout.
	int getfnum = 0; //assume-se que o nodo existe e não existe nodo 0.
	int a = atoi(nodo);
	int nouts;


	for(i=0;i<MAX_SIZE-1;i++) { 
		if (fin[i] == a) { getfnum = a; break; }//não é preciso continuar o ciclo :) }
	}


	//mais de 1 out

	if(fnouts[getfnum] > 1) { 
		nouts = fnouts[getfnum]; //numero de outs
		char *outs[10];
		//char *tmp[nouts];
		//tmp = fouts[getfnum]; //- TA A DAR WARNING, porquê ?
		//passar os outs sem o que se tem de remover
		//char *lixo[100];
		//sprintf(lixo, "valor de nouts: %d e de fouts[getfnum][0]: %s\n", nouts, fouts[getfnum][i]);
		//write(1,lixo,strlen(lixo));
		int m =0;
		//ESTA AQUI A DAR SEGMENTATION FAULT!!!! ao tentar copiar os valores, nao consigo descobrir o porquê
		for(i=0;i<nouts;i++) {
			//printf("i chegou= %d e %s \n",m,fouts[getfnum][0]);
			if(strcmp(fouts[getfnum][i],nodo) != 0) { outs[m] = fouts[getfnum][i];  m++ ; }
			//printf("fiz a atribuicao %d\n",m);
	
		}
		
		nouts--; //remover 1 ao total de outs
		kill(fpid[getfnum], SIGUSR1); //mandar parar na prox it
		waitpid(fpid[getfnum],NULL,0); //esperar que termine
		write(1,"Removido com sucesso\n",20); //mensagem de aviso com sucesso?
		//fazer novo connect (seria repetir parte do codigo do connect)
		connect(nodo, outs, nouts);
	}
	//só tem 1 out
	else {
		//ver PID do fanout
		printf("Pid a matar: %d pid deste processo: %d\n", fpid[getfnum],getpid());
		kill(fpid[getfnum], SIGUSR1); //mandar parar na prox it - É aqui que falha?
		waitpid(fpid[getfnum],NULL,0); //esperar que termine
		write(1,"Removido com sucesso\n",20); //mensagem de aviso com sucesso?
	}

}

// ########### DISCONNECT  END ############################################

// ######### INJECT ###########################
// inject <id> <cmd> <args...>

void inject(char *nodo, char *args[]) {
	//verificar se nodo existe e dar erro?

	//fork, redirect output para fifo nodo, só isto?

}


// ########## INJECT END #########################



    //testar criar nodos
    char *test[2] = {"./const","10"};
    char *test2[3] = {"./const","20"};
    char *saidas[2] = {"2","3"};
    char *saida[1] = {"3"};
    char *x[1] = {"2"};
    //execvp(test[0],test);

    node("1",test,0);
   	node("2",test2,0);
   	node("3",test,0);
    //node("1",test2,0); //erro ao criar
    //printf("criei os nodos\n");

    /*
    int fdin;
    mkfifo("/tmp/1out",0666);
    fdin = open("/tmp/1out",O_RDONLY);//abrir fifo entrada leitura
    if(fdin < 1) perror("Falhou o open no fanout");
    else write(fdin,"teste",5); */
    //connect("1",saidas,2); // connectar output nodo 1 ao input nodo 2 e 3, quantidade de nodos
    connect("1",saida,1); //connectar output nodo 1 ao input nodo 3
    //connect("1",)
    //disconnect("1","2"); //desconectar os o 2 do 1.
    //printf("connect feito, inserir colunas de teste:\n");
    
    //simular injfect nodo1
    int fdt = open("./tmp/1in",O_WRONLY);
    while((n=read(0,buffer,PIPE_BUF))>=0) {
    	if(n == 0) { write(1,"vou fazer disconnect\n",20); disconnect("1","3"); } //ctrl+d para eof
    	else write(fdt,buffer,n);
    }
    return 0;
}

/*
./controlador
./controlador ficheiro_configuracao.txt

./cliente node 1 window 2 avg 10
./cliente inject ...
*/
