#include <stdio.h>    // perror
#include <sys/stat.h> // mkfifo
#include <fcntl.h>    // open
#include <unistd.h>   // read
#include <string.h>   // strcmp, strtok
#include <stdlib.h>   // atoi
#include <limits.h>   // PIPE_BUF
#include <signal.h>   // sinais
#include <sys/wait.h> // wait

#include "readln.h"

#define MAX_SIZE PIPE_BUF
//########## PIPE_BUF

/******************************************************************************
 *                           VARIÁVEIS GLOBAIS                                *
 ******************************************************************************/

//############### PODE-SE USAR SO O NODESPID
int nodes[MAX_SIZE];    // array que indica se nó existe na rede
int nodespid[MAX_SIZE]; // array com os PIDs dos nós

int stopfan = 0; // serve para parar o fanout (conexão entre os nós) sem ser
                 // necessário fazê-lo abruptamente (i.e. com SIGKILL)

/*
 * Estrutura que configura um fanout
 */
typedef struct fanout {
    int pid;     // pid do fanout
    int* outs;   // array de IDs dos nós do output
    int numouts; // número de nós de output
} *Fanout;

/*
 * Vetor de fanouts que corresponde ao conjunto de todas as conexões entre nós
 * da rede. O índice deste array indica o ID do nó IN do fanout.
 * 
 * Se o fanout que tem o nó X como IN estiver ativo, a posição X do array é
 * diferente de NULL e tem uma struct do tipo fanout, corresponde à conexão
 * (fanout) que parte deste mesmo nó.
 */
Fanout connections[MAX_SIZE];
                              
/*
 * @brief Inicializa as variáveis globais da rede
 *
 * Iniciliza os nós a 0 e as conexões a NULL.
 */
void init_network()
{
    int i;

    for (i = 0; i < MAX_SIZE; i++) {
        nodes[i] = 0;
        connections[i] = NULL;
    }
}

void desbloqueia(int n)
{
    int fd;
    char fifo[15];

    sprintf(fifo, "./tmp/%dout", n);
    fd = open(fifo, O_WRONLY);
    write(fd, "-", 1);
}

/******************************************************************************
 *                          FUNÇÕES AUXILIARES                                *
 ******************************************************************************/

/*
 * @brief Cria um Fanout (struct)
 *
 * @param pid     PID do processo que corre o fanout
 * @param outs    Array com os IDs dos nós do output
 * @param numouts Número de nós do output
 */
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

/*
 * @brief Coloca a variável global stopfan a 1
 *
 * Esta função é usada como handler do sinal SIGUSR1 recebido pelo processo que
 * executa o fanout, fazendo com que este pare de executar na sua próxima iteracção (em alternativa a
 * matar diretamente o processo).
 */

void stop_fanout() { stopfan = 1; }

/*
 * @brief Executa um fanout
 *
 * Definimos como fanout uma função que recebe um input e repete o que conseguir
 * ler desse input para um ou mais outputs recebidos como parâmetro.
 *
 * Quando se quiser matar um fanout, é recebido um SIGUSR1 que coloca a variável
 * global stopfan a 1, fazendo parar o ciclo de escrita nas saídas. Com isto,
 * evita-se matar o processo abruptamente (i.e. com recurso ao SIGKILL) e
 * interromper o processo de escrita a meio.
 *
 * @param input   Input do fanout
 * @param outputs Array com os outputs
 * @param numouts Número de outputs
 */
void fanout(int input, int outputs[], int numouts)
{
    int i, fdi, fdos[numouts], bytes;
    char in[15], out[15], buffer[MAX_SIZE], aux[5];

    signal(SIGUSR1, stop_fanout);

    sprintf(aux, "%d", input);
    sprintf(in, "./tmp/%sout", aux);
    //printf("Vou abrir o fifo de input do fannout: %s \n",in); 
    fdi = open(in, O_RDONLY);
    
    if (fdi == -1) perror("open fifo in fanout");

    // Abrir FIFOs de saída
     for(i=0;i<numouts;i++) {
       //printf("outputss[i] do fannout: %d i: %d\n", outputs[i],i);
   }

	
    for (i = 0; i < numouts; i++) {
        sprintf(aux, "%d", outputs[i]);
        sprintf(out, "./tmp/%sin", aux);
	    fdos[i] = open(out, O_WRONLY);
	    if (fdos[i] == -1) perror("open fifo out fanout");
    }
    
    /* Escrever nos FIFOs de saída */

    while (!stopfan && (bytes = read(fdi, buffer, PIPE_BUF)) > 0) {
    	if (!stopfan) {
            for (i = 0; i < numouts; i++) {    
/*       	
			 char bufs[PIPE_BUF];
            sprintf(bufs,"fannout leu: %s de %d\n",buffer,fdi);
   	    	write(1,bufs,strlen(bufs));
            sprintf(bufs,"fannout vai escrever: %s no %d\n",buffer,fdos[i]);
   			write(1,bufs,strlen(bufs));   
*/
                write(fdos[i], buffer, bytes);
            }
    	}
    }
    
    _exit(0);
}


/******************************************************************************
 *                        COMANDOS DO CONTROLADOR                             *
 ******************************************************************************/

/*
 * @brief Comando que adiciona um nó à rede
 *
 * Primeiro, esta função verifica se o nó já existe na rede (se não existir dá
 * erro). Depois cria um processo filho para executar o componente/filtro, bem
 * como dois FIFOs (pipes com nome) de entrada e saida de dados no nó. Os nomes
 * destes pipes são "Xin" e "Xout" em que X é o ID do nó.
 *
 * Por fim, adiciona o nó criado à rede.
 *
 * @param options Array com os campos do comando (secções separadas por espaço)
 * @param flag    Flag que indica se o output do nó deverá ser descartado
 *
 * @return 0 em caso de sucesso ou 1 em caso de erro
 */

// e.g. node 1 window ...
int node(char** options, int flag)
{
    int n;

    /* Verificar se o nó já existe na rede */

    n = atoi(options[1]); // nó de entrada
    
    if (nodes[n] != 0) {
        printf("Já existe nó com ID %d\n", n);
        return 1;
    }

    /* Criar filho para correr o componente */

    nodespid[n] = fork();
    
    if (nodespid[n] == -1) perror("fork no node");
    
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
        
        /* Executar o componente e adicionar ./ ao filtro */
        char tmp[10];
        sprintf(tmp, "./%s", options[2]); //## testar
        options[2] = tmp;
		execvp(options[2], &options[2]);
    }

    /* Acrescentar nó à rede */   
        char tmp[10];
        sprintf(tmp, "./%s", options[2]); 
        options[2] = tmp;
        //write(1,options[2],strlen(options[2]));

    nodes[n] = 1;
    
    return 0;
}

/*
 * @brief Comando que faz a conexão entre dois ou mais nós da rede
 *
 * Primeiro verifica se já existia uma conexão cujo IN seja igual ao recebido em
 * options. Em caso afirmativo, guarda os IDs dos nós do output dessa conexão e
 * mata a conexão. Deixando terminar qualquer escrita que esteja a ser feita.
 * 
 * Em todos os casos (caso existisse ou não uma conexão anterior), são
 * adicionados os nós de output recebidos (em options) e é criada uma nova
 * conexão que liga os nós recebidos (mais os nós pré-existentes, caso seja esse
 * o caso).
 *
 * @param options    Array com campos do comando (secções separadas por espaço)
 * @param numoptions Tamanho do array com os campos do comando (options)
 *
 * @return 0 em caso de sucesso ou 1 em caso de erro 
 */
int connect(char** options, int numoptions)
{
    int i, j = 0, n, pid, numouts;
    Fanout f;

    n = atoi(options[1]);

    numouts = numoptions - 2;
    int outs[numouts]; // ver isto

    /* guardar valores dos outs actuais */
    if (connections[n] != NULL) {
        numouts += connections[n]->numouts;
        //int outs[numouts]; é aqui que f**** tudo, inicializava o array com indice 0 a 0.
        //printf("Valor numouts: %d e connections[n]->numouts: %d ;\n",numouts,connections[n]->numouts);

        for (i = 0; i < connections[n]->numouts; i++) {
        	outs[j] = connections[n]->outs[i];
        	//printf("sacou o valor do fannout: outs[%d] = %d,\n",j,outs[j]);
            j++;
   		}

        kill(connections[n]->pid, SIGUSR1);
        desbloqueia(n);
        waitpid(connections[n]->pid, NULL, 0);
        connections[n] = NULL;
    }
    /* adicionar os novos outs */
    for (i = 2; i < numoptions; i++) {
        outs[j] = atoi(options[i]);
       // printf("Valor de outs[j] final j %d = %d\n",j, outs[j]);
        j++;
    }

    //for(i=0;i<numouts;i++) {
    //	printf("Valor de outs[%d] = %d",i,outs[i]);
    //}

    pid = fork();

    if (pid == -1) { perror("fork no connect"); return 1; }
    if (pid == 0) {  fanout(n, outs, numouts); }
    else {
        f = create_fanout(pid, outs, numouts);
        connections[n] = f;
    }
    
    return 0;
}

/*
 * @brief Comando que desfaz a conexão entre dois nós da rede
 *
 * Primeiro verifica se existia alguma conexão para o IN recebido em options
 * (caso não haja é retornado erro). Depois, verifica se esse IN tem o OUT
 * recebido em options como output (caso não tenha é retornado erro). Após isso,
 * se apenas houver esse OUT na conexão pré-existente, então essa conexão é
 * terminada e a função termina. Caso contrário, são guardados os restantes outs
 * da conexão pré-existente e é criada uma nova conexão com apenas esses outs
 * (e sem o OUT retirado).
 *
 * @param options Array com os campos do comando (secções separadas por espaço)
 *
 * @return 0 em caso de sucesso ou 1 em caso de erro, 2 quando os nodos não
 *         estarem conectados
 */
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
            return 2;
        }

        if (numouts == 1) {
            kill(connections[a]->pid, SIGUSR1);
            desbloqueia(a);
            waitpid(connections[a]->pid, NULL, 0);
            connections[a] = NULL;         
            return 0;
        }
        else {
            int outs[numouts--];

            for (i = 0; i < connections[a]->numouts; i++) {
                if (connections[a]->outs[i] != b) {
                    outs[j] = connections[a]->outs[i]; 
                    j++;
                }
            }

            kill(connections[a]->pid, SIGUSR1);
            desbloqueia(a);
            waitpid(connections[a]->pid, NULL, 0);
            connections[a] = NULL;

            pid = fork();

            if (pid == -1) perror("fork no node");

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
        return 2;
    }

    return 0;
}

/*
 * @brief Comando que injeta a entrada de um nó da rede com o resultado da
 *        execução de um outro comando (do sistema Unix)
 * 
 * Abre o FIFO de entrada do nó recebido em options e, de seguida, cria um filho
 * que execute o comando e escreve lá o resultado da execução do mesmo.
 *
 * @param options Array com os campos do comando (secções separadas por espaço)
 *
 * @return 0 em caso de sucesso ou 1 em caso de erro
 */
int inject(char** options)
{
    int fd, pid;
    char in[15];

    sprintf(in, "./tmp/%sin", options[1]);

    /* abre fifo in do node para escrita  */
    fd = open(in, O_WRONLY);

    if (fd == -1) { perror("open inject"); return 1; }

    pid = fork(); 

    if (pid == -1) { perror("fork inject"); return 1; }

    if (pid == 0) {
        dup2(fd, 1);
        execvp(options[2], &options[2]);
        perror("exec inject");
        return 1;
    }

    return 0;
}

/* remove um nodo *****
remove um nodo da rede passado como argumento
vai verificar se está a receber algum output ou se é a fonte de algum input e se for o caso remove dessa rede
já existe uma função remove no stdio.h e por isso ficou chamada apaga



******************************/

int apaga(char** options) {

    int i,in[10],out[10],t=0, ntemp,j,pid;
    char tempin[10], tempout[10];
    int a = atoi(options[1]);
    char* tempstr[30];

    //* verificar se está conectado a algum nodo */

    if (connections[a] != NULL) { 
        /* existe como entrada, é só remover */
        kill(connections[a]->pid, SIGUSR1);
        desbloqueia(a);
        waitpid(connections[a]->pid, NULL, 0);
        connections[a] = NULL; 
    } 

    for(i = 0; i < MAX_SIZE; i++) {
        if (connections[i] != NULL) { 
            ntemp = connections[i]->numouts;
            for(j = 0; j < ntemp; j++) { /* verificar todas as saidas existentes nos fannouts */
                if(connections[i]->outs[j] == a) { 
                    //está a criar o *options com disconnect x y
                    tempstr[0] = "disconnect";
                    sprintf(tempstr[1], "%d", i);
                    //tempstr[1] = toString(i);
                    tempstr[2] = options[1];
                    disconnect(tempstr);
            	}
         	} 
    	}
    }

    /* já está removido de qualquer ligação com o resto da rede, terminar a sua existência e fechar fifos */
    sprintf(tempin,"./tmp/%sin",options[1]);
    sprintf(tempout,"./tmp/%sout",options[1]);
    if(!fork()) { execlp("rm","rm",tempin,NULL); }
    if(!fork()) { execlp("rm","rm",tempout,NULL); }
    kill(nodespid[a],SIGKILL);
    nodes[a] = 0; //mover para só um array e meter NULL ou 0.
    //printf("valor de nodes[%d] = %d\n",a,nodes[a]);
    //printf("Node %s removido com sucesso\n",options[1]);
    return 0;
}

/* muda o filtro dum node ********************************************************************


*******/

int change(char** options, int flag) {
    int a = atoi(options[1]); 
	int totalouts,i,pid;
	int newouts[10];
	Fanout f;

    /* verifica se é um nodo de saida */
    if(connections[a] != NULL) {
    	totalouts = connections[a]->numouts;
    	//newouts = connections[a]->outs;
    	for(i=0;i<totalouts;i++) {
    		newouts[i] = connections[a]->outs[i];
    	}
	
	    apaga(options); 
	   	node(options, flag);
	    /* executa novo fannout */
	    pid = fork();

	    if (pid == -1) { perror("fork no connect"); return 1; }
	    if (pid == 0) { fanout(a, newouts, totalouts); }
	    else {
	        f = create_fanout(pid, newouts, totalouts);
	        connections[a] = f;
	    }
    }
    else { /* não é nodo de saida */
    //###### aqui vai dar barraca porque vai fechar fifos que não vão ser abertos de novo com um fannout, ver saidas, quando encontrar, matar esse fannout e correr de novo no fim de criar o nodo.
    //#### alternativa, ver saidas e fazer disconnect e connect de novo.
    	apaga(options); 
	   	node(options, flag);
    }
    return 0;
}



/******************************************************************************
 *                      INTERPRETADOR DE COMANDOS                             *
 ******************************************************************************/

/*
 * @brief Interpretador dos comandos do controlador
 *
 * @param cmdline Comando recebido
 *
 * @return 0 em caso de sucesso ou 1 em caso de erro
 */
int interpretador(char* cmdline)
{
    int i = 0, ret;
    char* options[MAX_SIZE];

    /* Separa a linha recebida pelos espaços */

    options[i] = strtok(cmdline, " ");

    while (options[i] != NULL) { options[++i] = strtok(NULL, " "); }

    /* Interpreta qual o comando e invoca a função respetiva */

    //####### adicionar apaga e change

    if (strcmp(options[0], "node") == 0) {
        if (strcmp(options[2], "const") && strcmp(options[2], "filter") && strcmp(options[2], "window") && strcmp(options[2], "spawn")) {
            ret = node(options, 1); 

            if (ret == 0) { printf("Nó criado com sucesso\n"); }
        }
        else {
            ret = node(options, 0);

            if (ret == 0) { printf("Nó criado com sucesso\n"); }
        }

        return ret;
    }

    else if (strcmp(options[0], "connect") == 0) {
        ret = connect(options, i);

        if (ret == 0) {  printf("Nós conectados com sucesso\n"); }

        return ret;
    }

    else if (strcmp(options[0], "disconnect") == 0) {
        ret = disconnect(options);

        if (ret == 0) { printf("Nós disconectados com sucesso\n"); }
        
        else if (ret == 2) { printf("Nós não estavam conectados\n");  }

        return ret;
    }

    else if (strcmp(options[0], "inject") == 0) {
        ret = inject(options);

        if (ret == 0) {  printf("Inject executado com sucesso\n");  }

        return ret;
    }

    else if (strcmp(options[0], "remove") == 0) {
        ret = apaga(options);

        if (ret == 0) {  printf("Nó removido com sucesso\n");  }
        
        return ret;
    }

    else if (strcmp(options[0], "change") == 0) {
        if (strcmp(options[2], "const") && strcmp(options[2], "filter") && strcmp(options[2], "window") && strcmp(options[2], "spawn")) {
            ret = change(options, 1);

            if (ret == 0) { printf("Comando do nó alterado com sucesso\n"); }
        }
        else {
            ret = change(options, 0);

            if (ret == 0) { printf("Comando do nó alterado com sucesso\n"); }
        }

        return ret;
    }
    //TESTES - escreve input e entras aqui, ctrl+d para regressar ao menu
    //escreve sempre no node 1, portanto faz connects sempre com 1 x
	else if (strcmp(options[0], "input") == 0) {
		int fdp,p;
		char backs[MAX_SIZE];
		fdp = open("./tmp/1in", O_WRONLY);
		write(1,"MODO DE INPUT\n",14);
		while(((p = read(0,backs,PIPE_BUF)) > 0)) {
			write(1,backs,p);
			write(fdp,backs,p);
		}
		write(1,"Sai do input\n",14);
	}
	//FIM TESTES
    else {
    	//########## ACABAR ISTO
    	printf("Comando inexistente\nTente com Node X filtro <ops..>\nconnect x y (...)\ninject x <ops>\netc...\n");
    }

    return 0;
}


/******************************************************************************
 *                                 MAIN                                       *
 ******************************************************************************/

/*
 * @brief Função main do controlador
 *
 * O controlador pode ser, opcionalmente, invocado com a referência a um
 * ficheiro de configuração. Neste caso, este ficheiro é lido e os comandos são
 * interpretados. Em todos os casos, de seguida o controlador permanece em
 * execução, à espera que receba mais comandos do stdin.
 *
 * O resultado final da aplicação dos componentes/filtros aos dados injetados é
 * apresentado no stdout.
 *
 * @return 0 em caso de sucesso ou 1 em caso de erro
 */
int main(int argc, char* argv[])
{
    int fd,n;   
    char buffer[MAX_SIZE];

    /* Inicializa as variáveis globais da rede */

    init_network();

    /* Caso seja passado um ficheiro de configuração como argumento, este é lido
       e os comando são interpretados sequencialmente (linha a linha) */

    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        //############# Nao testei ler de ficheiro
        while (readln(fd, buffer, MAX_SIZE) > 0) {
            interpretador(buffer);
        }
    }

    /* Lê comandos do stdin até receber EOF (Ctrl-D) */

    //criar coisas para teste

    write(1,"BEM VINDO AO MEGA GERENCIADOR DE REDES\n",39);
    while ((n = read(0, buffer, MAX_SIZE)) > 0) {
        buffer[n-1] = '\0'; //remover \n
        interpretador(buffer);
    }

    return 0;
}