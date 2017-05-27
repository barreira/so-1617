#include <stdio.h>    // perror
#include <sys/stat.h> // mkfifo
#include <fcntl.h>    // open
#include <unistd.h>   // read
#include <string.h>   // strcmp, strtok
#include <stdlib.h>   // atoi
#include <limits.h>   // PIPE_BUF
#include <signal.h>   // sinais
#include <sys/wait.h> // wait

#define MAX_SIZE PIPE_BUF
//########## PIPE_BUF

/******************************************************************************
 *                           VARIÁVEIS GLOBAIS                                *
 ******************************************************************************/

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


/******************************************************************************
 *                          FUNÇÕES AUXILIARES                                *
 ******************************************************************************/

/*
 * @brief Lê uma linha
 *
 * @param fildes Descritor de ficheiro de onde se lê
 * @param buf    Buffer para onde se escreve os dados lidos
 * @param nbyte  Número máximo de bytes a ler
 *
 * @return Retorna o número de bytes lidos
 */
ssize_t readln(int fildes, void *buf, size_t nbyte) {
	int i, n;

	for (i=0; i < nbyte; i++) {
		n = read(fildes, buf+i, 1);

		if (n == -1) return -1; //########## aqui não pode dar -1 ou o interpretador pára. certo?

		if (n == 0) break;

		if (((char*)buf)[i] == '\n')
			((char *)buf)[i] = '\0';
	}

	return ++i;
}

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
 * executa o fanout, fazendo com que este pare de executar (em alternativa a
 * matar diretamente o processo).
 */
void stop_fanout()
{
    stopfan = 1;
}

/*
 * @brief Executa um fanout
 *
 * Definimos como fanout uma função que recebe um input e repete o que conseguir
 * ler desse input para um ou mais outputs recebidos como parâmetro.
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
    printf("Vou abrir o fifo de input do fannout: %s \n",in);
    fdi = open(in, O_RDONLY);
    
    if (fdi == -1) perror("open fifo in fanout");

    // Abrir FIFOs de saída
    for(i=0;i<numouts;i++) {
        printf("outputss[i] do fannout: %d i: %d\n", outputs[i],i);
    }

    for (i = 0; i < numouts; i++) {
        sprintf(aux, "%d", outputs[i]);
        sprintf(out, "./tmp/%sin", aux);
        //printf("Vou abrir as saidas fifo out do fanout: %s \n",out);
	    fdos[i] = open(out, O_WRONLY);
	    if (fdos[i] == -1) perror("open fifo out fanout");
    }
    /* 
    connect 1 2 3
Vou abrir o fifo de input do fannout: ./tmp/1out 
Vou abrir as saidas fifo out do fanout: ./tmp/0in 
open fifo out fanout: No such file or directory
Vou abrir as saidas fifo out do fanout: ./tmp/2in 
Vou abrir as saidas fifo out do fanout: ./tmp/0in 
open fifo out fanout: No such file or directory
Vou abrir as saidas fifo out do fanout: ./tmp/2in 

deveria abrir o 2 e 3 in
*/
    // Escrever nos FIFOs de saída

    while ((bytes = read(fdi, buffer, PIPE_BUF)) > 0 && !stopfan) {
        for (i = 0; i < numouts; i++) {
            write(fdos[i], buffer, bytes);
        }
    }
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

//######### criar nodo sem output, exec das options sem alterar
int node(char** options, int flag)
{
    int n;

    /* Verificar se o nó já existe na rede */

    n = atoi(options[1]); // nó de entrada
    printf("valor de options[1]: %s \n",options[1]);
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

        sprintf(in, "./tmp/%sin", options[1]); //###########está a escrever lixo
        if (!flag) { sprintf(out, "./tmp/%sout", options[1]); } //##########está a escrever lixo

        mkfifo(in, 0666);
        if (!flag) { mkfifo(out, 0666); } //#########descartar output, faz-se na mesma para depois ao remover não dar erro?
        

        fdi = open(in, O_RDONLY);

        if (!flag) { fdo = open(out, O_WRONLY); } //#######descartar output, faz-se na mesma para depois ao remover não dar erro?
        
        /* Redirecionar para os FIFOs */

        dup2(fdi, 0);
        if (!flag) { dup2(fdo, 1); } //este tem de ficar sempre.
        
        /* Executar o componente */
        //########### ao passar o componente tem de se por o ./ no options[2]
        //execvp(options[2], &options[3]);
        execlp("./const","./const","10",NULL);
    }
    
    /* Acrescentar nó à rede */   

    nodes[n] = 1;
    //write(1,"Node Criado com sucesso\n",25);
    return 0;
}

/*
 * @brief Comando que faz a conexão entre dois ou mais nós da rede
 *
 * Primeiro verifica se já existia uma conexão cujo IN seja igual ao recebido em
 * options. Em caso afirmativo, guarda os IDs dos nós do output dessa conexão e
 * mata a conexão.
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
    
    for (i = 2; i < numoptions; i++) {
        //printf("Options[i] = %s ; i= %d\n",options[i],i);
        outs[j] = atoi(options[i]);
        j++;
    }

    pid = fork();

    if (pid == -1) perror("fork no connect");
    
    if (pid == 0) {     
        fanout(n, outs, numouts);
    }
    else {
        f = create_fanout(pid, outs, numouts);
        connections[n] = f;
    }
    write(1,"Fannout executado com sucesso\n",30);
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
 * @return 0 em caso de sucesso ou 1 em caso de erro
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
        return 1;
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

// ###### Falta adicionar o envio dum sinal a todos os nodos para tirar do pause.
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

/* remove um nodo 
remove um nodo da rede
vai verificar se está a receber algum output ou se é a fonte de algum input e se for o caso remove dessa rede
já existe uma função remove no stdio.h
*/

int apaga(char** options) {

    int i,in[10],out[10],t=0, ntemp,j,pid;
    char tempin[10], tempout[10];
    int a = atoi(options[1]);
    //ver se tem in e/ou out no fanout
    if (connections[a] != NULL) { 
        //se sim, in: remover fanout
        kill(connections[a]->pid, SIGUSR1);
        waitpid(connections[a]->pid, NULL, 0);
        connections[a] = NULL; 
    } 
    // verificar em todos os outs
    /* Penso que seja este o codigo, verificar para todos os fannouts se algum tem aquela saida.
    for(i=0;i<MAX_SIZE;i++) {
        if (connections[i] != NULL) { 
            ntemp = connections[i]->numouts;
            for(j=0;j<ntemp;j++) { //ver dentro dos outs
                if(connections[i]->outs[j] == a) { 
                //########remover do array aquele valor
                //########matar fannout e criar nova
                //out[t] = connections[i]->outs[j] ; break } //só tem 1 out
                t++;
            }
         } 
    }
	*/
    //remover os fifos e matar o node
    sprintf(tempin,"./tmp/%sin",options[1]);
    sprintf(tempout,"./tmp/%sout",options[1]);
    if(!fork()) { execlp("rm","rm",tempin,NULL); }
    if(!fork()) { execlp("rm","rm",tempout,NULL); }
    kill(nodespid[a],SIGKILL);
    nodes[a] = 0;
    //printf("Node %s removido com sucesso\n",options[1]);
    return 0;
}

/* muda o filtro dum node ***************************************************************************/

int change(char** options, int flag) {
    int a = atoi(options[1]); 
    apaga(options); //apaga(nodo x) assumindo que o apaga trata de tudo em condiçoes
    //options++;
    node(options, flag); //criar o novo com a descartar ou não output - trabalho feito no interpretador de decifrar o comando
    printf("Node %s alterado com sucesso\n", options[1]);
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
    int i = 0;
    char* options[MAX_SIZE];

    /* Separa a linha recebida pelos espaços */

    options[i] = strtok(cmdline, " ");

    while (options[i] != NULL) {
        options[++i] = strtok(NULL, " ");
    }

    /* Interpreta qual o comando e invoca a função respetiva */

    //############# se não for nenhum dos filtros, fazer node com flag a 1.
    //####### add change e remove

    if (strcmp(options[0], "node") == 0) {
    	//###################### falta isto
    	// if options[2] != spawn && filter && window && const => output descarta-se
    	//função para fazer isto:
    	//fork()
    	//execlp(...)
    	//else
        if(!node(options, 0)) printf("Node criado com sucesso\n");
        //int node(char** options, int flag)
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
    else if (strcmp(options[0], "apaga") == 0) {
        if(!apaga(options)) printf("removido com sucesso\n");
    }
    else if (strcmp(options[0], "change") == 0) {
        return change(options, 0); //está assim só para testes, tem de se ver se o nodo vai ter saida ou não.
    }

    else { 
    	//########## adicionar remove, change e por mais bonito :P
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

        while (readln(fd, buffer, MAX_SIZE) > 0) {
            interpretador(buffer);
        }
    }

    /* Lê comandos do stdin até receber EOF (Ctrl-D) */

    //criar coisas para teste

    write(1,"BEM VINDO AO MEGA GERENCIADOR DE REDES\n",39);
    while ((n = read(0, buffer, MAX_SIZE)) > 0) {
        buffer[n-1] = '\0';
        interpretador(buffer);
    }

    return 0;
}