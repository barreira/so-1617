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

#define MAX_SIZE   PIPE_BUF //########## PIPE_BUF
#define SMALL_SIZE 32


/******************************************************************************
 *                           VARIÁVEIS GLOBAIS                                *
 ******************************************************************************/

//############### PODE-SE USAR SO O NODESPID, cuidado ao alterar se bate certo.
int nodes[MAX_SIZE];    /* array que indica se nó existe na rede */
int nodespid[MAX_SIZE]; /* array com os PIDs dos nós */

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

/*
 * @brief Desbloqueia FIFO para que ele termine
 *
 * Esta função faz uma escrita de um caratere para o FIFO do nó cujo ID é
 * passado como parâmetro. Serve para desbloquear a leitura/escrita do FIFO e
 * permitir que este termine com sucesso.
 *
 * @param n ID do nó cujo FIFO se pretende desbloquear
 *
 */
void desbloqueia(int n)
{
    int fd;
    char fifo[SMALL_SIZE];

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
 * executa o fanout, fazendo com que este pare de executar na sua próxima
 * iteração (em alternativa a matar diretamente o processo com um SIGKILL).
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
    char in[SMALL_SIZE], out[SMALL_SIZE], buffer[MAX_SIZE], aux[SMALL_SIZE];

    signal(SIGUSR1, stop_fanout);

    /* Gerar o nome do FIFO e abri-lo */

    sprintf(aux, "%d", input);
    sprintf(in, "./tmp/%sout", aux); 
    fdi = open(in, O_RDONLY);
    
    if (fdi == -1) perror("open fifo in fanout");

    /* Abrir FIFOs de saída */

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
 *        e.g. node <id> <cmd> <args...>
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
 * @return 0 em caso de sucesso
 *         1 em caso de erro
 *         2 caso já exista o nó na rede
 */
int add_node(char** options, int flag)
{
    int n;

    /* Verificar se o nó já existe na rede */

    n = atoi(options[1]);
    
    if (nodes[n] != 0) {
        return 2;
    }

    /* Criar filho para correr o componente */

    nodespid[n] = fork();
    
    if (nodespid[n] == -1) perror("fork no node");
    
    if (nodespid[n] == 0) {
        
        /* Fazer FIFOs in e out e abri-los */

        char in[SMALL_SIZE], out[SMALL_SIZE];
        int fdi, fdo;

        sprintf(in, "./tmp/%sin", options[1]);
        if (!flag) sprintf(out, "./tmp/%sout", options[1]);

        mkfifo(in, 0666);
        if (!flag) mkfifo(out, 0666);

        fdi = open(in, O_RDONLY);

        if (!flag) fdo = open(out, O_WRONLY);
        
        /* Redirecionar para os FIFOs */

        dup2(fdi, 0);
        if (!flag) dup2(fdo, 1);
        
        /* Adicionar "./" ao nome do componente e executá-lo */

        if (!flag) {
            char cmd[SMALL_SIZE];
            sprintf(cmd, "./%s", options[2]);
            options[2] = cmd;
        }

		execvp(options[2], &options[2]);
    }

    /* Acrescentar o nó à rede */

    nodes[n] = 1;
    
    return 0;
}

/*
 * @brief Comando que faz a conexão entre dois ou mais nós da rede
 *
 *        e.g. connect <id> <ids...>
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
 * @return 0 em caso de sucesso
 *         1 em caso de erro 
 */
int connect(char** options, int numoptions)
{
    int i, j = 0, n, pid, numouts;
    Fanout f;

    n = atoi(options[1]); // ID do nó IN recebido (em options)

    numouts = numoptions - 2; // número de OUTS recebido corresponde ao tamanho
                              // de options (numoptions), subtraido de 2:
                              // "connect" (options[0]) e "<nó IN>" (options[1])

    int outs[numouts]; // array que conterá os IDs dos OUTS da nova conexão

    /* Caso já exista uma conexão a partir do IN recebido (em options) */

    if (connections[n] != NULL) {

        /* Guarda-se os OUTS da conexão pré-existente */

        numouts += connections[n]->numouts;

        for (i = 0; i < connections[n]->numouts; i++) {
        	outs[j] = connections[n]->outs[i];
            j++;
   		}

        /* Mata-se o processo da conexão pré-existente */

        kill(connections[n]->pid, SIGUSR1);
        desbloqueia(n);
        waitpid(connections[n]->pid, NULL, 0);
        connections[n] = NULL;
    }

    /* Adiciona-se os novos OUTS ao array outs */
    
    for (i = 2; i < numoptions; i++) {
        outs[j] = atoi(options[i]);
        j++;
    }

    /* Cria-se o processo da nova conexão com o array de outs criado,
       adicionando-o à lista global das conexões */

    pid = fork();

    if (pid == -1) { perror("fork no connect"); return 1; } 

    if (pid == 0) {
        fanout(n, outs, numouts);
    }

    else {
        f = create_fanout(pid, outs, numouts);
        connections[n] = f;
    }
    
    return 0;
}

/*
 * @brief Comando que desfaz a conexão entre dois nós da rede
 *
 *        e.g. disconnect <id1> <id2>
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
 * @return 0 em caso de sucesso
 *         1 em caso de erro
 *         2 caso os nós não estejam previamente conectados
 */
int disconnect(char** options)
{
    int a, b, numouts, pid, i, j = 0, exists = 0;
    Fanout f;

    a = atoi(options[1]);
    b = atoi(options[2]);

    /* Verificar se existe alguma conexão para o IN (a) recebido */

    if (connections[a] != NULL) {
        
        numouts = connections[a]->numouts;

        /* Verificar se a conexão tem OUT (b) como saída */

        for (i = 0; i < numouts; i++) {
            if (connections[a]->outs[i] == b) {
                exists = 1;
            }
        }

        if (!exists) { // A conexão não tem OUT (b) com saída
            return 2;
        }

        /* Se a conexão apenas tiver OUT (b) como saída, pode ser terminada 
           diretamente */

        if (numouts == 1) {
            kill(connections[a]->pid, SIGUSR1);
            desbloqueia(a);
            waitpid(connections[a]->pid, NULL, 0);
            connections[a] = NULL;
            return 0;
        }
        else {

            /* Guarda-se os OUTS da conexão pré-existente para todos os OUTS
               cujo ID seja diferente do ID do OUT que vamos retirar (b) */

            numouts--;
            int outs[numouts];

            for (i = 0; i < connections[a]->numouts; i++) {
                if (connections[a]->outs[i] != b) {
                    outs[j] = connections[a]->outs[i]; 
                    j++;
                }
            }

            /* Mata-se o processo da conexão pré-existente */

            kill(connections[a]->pid, SIGUSR1);
            desbloqueia(a);
            waitpid(connections[a]->pid, NULL, 0);
            connections[a] = NULL;

            /* Cria-se uma nova conexão com o array de outs criado anteriormente
               (sem o OUT que retirámos (b)) */

            pid = fork();

            if (pid == -1) { perror("fork no node"); return 1; }

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
 *        e.g. inject <id> <cmd> <args...>
 *
 * Abre o FIFO de entrada do nó recebido em options e, de seguida, cria um filho
 * que execute o comando e escreve lá o resultado da execução do mesmo.
 *
 * @param options Array com os campos do comando (secções separadas por espaço)
 *
 * @return 0 em caso de sucesso
 *         1 em caso de erro
 *         2 caso não exista o nó na rede
 */
int inject(char** options)
{
    int a, fd, pid;
    char in[SMALL_SIZE];

    /* Verificar se o nó recebido existe na rede */

    a = atoi(options[1]);

    if (nodes[a] == 0) {
        return 2;
    }

    /* Cria-se a string do FIFO IN do nó recebido, abrindo-o para escrita */

    sprintf(in, "./tmp/%sin", options[1]);

    fd = open(in, O_WRONLY);

    if (fd == -1) { perror("open inject"); return 1; }

    /* Cria-se o processo responsável pelo inject no FIFO IN do nó */

    pid = fork(); 

    if (pid == -1) { perror("fork inject"); return 1; }

    /* Redireciona-se o output do processo para o FIFO e executa-se o comando
       recebido */

    if (pid == 0) {
        dup2(fd, 1);
        execvp(options[2], &options[2]);
        perror("exec inject");
        return 1;
    }

    return 0;
}

/*
 * @brief Comando que remove um nó da rede
 *
 *        e.g. remove <id>
 *
 * Esta função remove todas as ligações do nó a remover existentes na rede,
 * matando, posteriormente, o processo que se encontrava a executar o processo 
 * relativo ao nó removido.
 *
 * @param options Array com os campos do comando (secções separadas por espaço)
 *
 * @return 0 em caso de sucesso
 *         1 em caso de erro
 *         2 caso o nó não exista na rede
 */
int remove_node(char** options) {

    int a, numouts, i, j;
    char in[SMALL_SIZE], out[SMALL_SIZE];
    char* args[SMALL_SIZE];

    /* Verificar se o nó recebido existe na rede */

    a = atoi(options[1]);

    if (nodes[a] == 0) {
        return 2;
    }

    /* Verificar se o nó recebido se encontra ligado a algum outro nó da rede
       (i.e. se existe alguma conexão a partir daquele nó). Se sim, mata-se essa
       conexão */

    if (connections[a] != NULL) { 
        kill(connections[a]->pid, SIGUSR1);
        desbloqueia(a);
        waitpid(connections[a]->pid, NULL, 0);
        connections[a] = NULL; 
    } 

    /* Percorrer todas as conexões da rede para encontrar aquelas que têm o nó
       que queremos remover como OUT */

    for (i = 0; i < MAX_SIZE; i++) {
        if (connections[i] != NULL) { 
            numouts = connections[i]->numouts;

            for (j = 0; j < numouts; j++) {

                /* Caso uma conexão tenha o nó como OUT, faz-se o disconnect
                   entre esse nó e o nó da entrada dessa conexão */

                if (connections[i]->outs[j] == a) {

                    /* Criar array de options do disconnect e sua executá-lo */

                    args[0] = "disconnect";
                    sprintf(args[1], "%d", i);
                    args[2] = options[1];
                    disconnect(args);
            	}
         	} 
    	}
    }

    /* Todas as conexões do nó foram removidas, por isso pode-se remover o nó da
       rede, matando o seu processo e fechando os seus FIFOs (apagando-os) */

    sprintf(in, "./tmp/%sin", options[1]);
    sprintf(out, "./tmp/%sout", options[1]);
    
    if (fork() == 0) execlp("rm", "rm", in, NULL);
    else perror("fork remove in");

    if (fork() == 0) execlp("rm", "rm", out, NULL);
    else perror("fork remove out");

    kill(nodespid[a], SIGKILL);
    nodes[a] = 0; // array dos nós da rede deixa de ter o nó que foi removido

    return 0;
}

/*
 * @brief Comando que altera o componente/filtro a ser executado por um nó da
 *        rede
 *
 *        e.g. change <id> <cmd> <args...>
 *
 * Caso exista, remove o nó pré-existente (com o mesmo ID) da rede e cria um
 * novo nó (também com o mesmo ID) que executará o novo comando, mantendo todas
 * as conexões previamente existentes.
 *
 * @param options Array com os campos do comando (secções separadas por espaço)
 * @param flag    Indica se o output do novo nó criado deve ser descartado
 *                (parâmetro da função add_node)
 *
 * @return 0 em caso de sucesso
 *         1 em caso de erro
 *         2 caso o nó não exista na rede
 */
int change(char** options, int flag) {
    int a, numouts, pid, i;
	Fanout f;

    /* Verificar se o nó recebido existe na rede */

    a = atoi(options[1]);

    if (nodes[a] == 0) {
        return 2;
    }

    /* Verificar se existe alguma conexão cuja entrada (IN) corresponda ao nó
       recebido */

    if (connections[a] != NULL) {

        /* Guardar os OUTS da conexão pré-existente */

    	numouts = connections[a]->numouts;
    	int outs[numouts];

    	for (i = 0; i < numouts; i++) {
    		outs[i] = connections[a]->outs[i];
    	}
	
        /* Remover o nó antigo da rede e adicionar um nó que executará o novo
           comando */

	    remove_node(options); 
	   	add_node(options, flag);

	    /* Criar um processo que executará a conexão (fanout) relativa à
           execução do novo comando do nó */

	    pid = fork();

	    if (pid == -1) {
            perror("fork change");
            return 1;
        }

	    if (pid == 0) {
            fanout(a, outs, numouts);
        }
	    
        else {
	        f = create_fanout(pid, outs, numouts);
	        connections[a] = f;
	    }
    }
    else { /* A saída do nó recebido não está ligada a mais nenhum nó da rede */
    // #ver isto
    //###### aqui vai dar barraca porque vai fechar fifos que não vão ser abertos de novo com um fannout, ver saidas, quando encontrar, matar esse fannout e correr de novo no fim de criar o nodo.
    //###### alternativa, ver saidas e fazer disconnect e connect de novo.
    	remove_node(options); 
	   	add_node(options, flag);
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
 * @return 0 em caso de sucesso
 *         1 em caso de erro
 *         2 em caso de erro na execução do comando
 */
int interpretador(char* cmdline)
{
    int i = 0, ret;
    char* options[MAX_SIZE];

    /* Separa a linha recebida pelos espaços */

    options[i] = strtok(cmdline, " ");

    while (options[i] != NULL) {
        options[++i] = strtok(NULL, " ");
    }

    /* Interpreta qual o comando, invocando a função respetiva */

    /* Node */

    if (strcmp(options[0], "node") == 0) {
        if (strcmp(options[2], "const") && strcmp(options[2], "filter") &&
            strcmp(options[2], "window") && strcmp(options[2], "spawn")) {

            ret = add_node(options, 1);
        }
        else {
            ret = add_node(options, 0);
        }

        if (ret == 0) printf("Nó criado com sucesso\n");

        return ret;
    }

    /* Connect */

    else if (strcmp(options[0], "connect") == 0) {
        ret = connect(options, i);

        if (ret == 0) printf("Nós conectados com sucesso\n");

        return ret;
    }

    /* Disconnect */

    else if (strcmp(options[0], "disconnect") == 0) {
        ret = disconnect(options);

        if (ret == 0) printf("Nós disconectados com sucesso\n");
        
        else if (ret == 2) printf("Erro: Os nós não se encontram conectados\n");

        return ret;
    }

    /* Inject */

    else if (strcmp(options[0], "inject") == 0) {
        ret = inject(options);

        if (ret == 0) printf("Inject executado com sucesso\n");

        return ret;
    }

    /* Remove */

    else if (strcmp(options[0], "remove") == 0) {
        ret = remove_node(options);

        if (ret == 0) printf("Nó removido com sucesso\n");
        
        return ret;
    }

    /* Change */

    else if (strcmp(options[0], "change") == 0) {
        if (strcmp(options[2], "const") && strcmp(options[2], "filter") &&
            strcmp(options[2], "window") && strcmp(options[2], "spawn")) {

            ret = change(options, 1);
        }
        else {
            ret = change(options, 0);
        }

        if (ret == 0) printf("Comando do nó alterado com sucesso\n");

        return ret;
    }

    /* Modo de teste (Ctrl-D para regressar ao menu) */

	else if (strcmp(options[0], "input") == 0) {
		int fdp, p;
		char backs[MAX_SIZE];

		fdp = open("./tmp/1in", O_WRONLY);

		write (1, "MODO DE INPUT\n", 14);
        while(((p = read(0, backs, PIPE_BUF)) > 0)) {
			write(1, backs, p);
			write(fdp, backs, p);
		}
		write (1,"Sai do input\n",14);
	}

    /* Comando lido não corresponde a nenhuma das opções possíveis */

    else {
    	printf("Erro: Comando inexistente\n");
        return 1;
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
 * interpretados.
 *
 * Em todos os casos, o controlador permanece em execução, à espera que receba
 * mais comandos do stdin.
 *
 * @return 0 em caso de sucesso
 *         1 em caso de erro
 */
int main(int argc, char* argv[])
{
    int fd, bytes;
    char buffer[MAX_SIZE];

    /* Inicializa as variáveis globais da rede */

    init_network();

    /* Caso seja passado um ficheiro de configuração como argumento, este é lido
       e os comando são interpretados sequencialmente (linha a linha) */

    if (argc == 2) { //# falta testar
        fd = open(argv[1], O_RDONLY);
        
        while (readln(fd, buffer, MAX_SIZE) > 0) {
            interpretador(buffer);
        }
    }

    /* Lê comandos do stdin até receber EOF (Ctrl-D) */

    while ((bytes = readln(0, buffer, MAX_SIZE)) > 0) {
        interpretador(buffer);
    }

    return 0;
}