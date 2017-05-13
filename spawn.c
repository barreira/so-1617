#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

/*spawn <cmd> <args...>
Este programa reproduz todas as linhas, executando o comando indicado uma vez para cada uma delas,
e acrescentando uma nova coluna com o respetivo exit status.
spawn mailx -s $3 x@y.com
substituir $n pelo valor da coluna

./a.out mailx -s \$3 x@y.com 


*/

int main(int argc, char const *argv[]){

	int total = argc-1;
	int coluna =0, achei=0; //guardar o $n
	int i,n,cut,s, pid,status;
	char *cmd[total]; //guarda o comando a executar
	char *tmp;
	char buffer[PIPE_BUF];
	char print[PIPE_BUF];
	char final[PIPE_BUF];
	char field[PIPE_BUF];

	//verificação de erros do numero de argumentos (ou assume-se que o input é sempre correcto?)
	if(argc < 2) { write(2,"Sem argumentos!!",16); return 2; }

	//passar argumentos para array ; remover ./a.out
	//--argc; ++argv;
	for(i=0;i<argc;i++) {
		    if(i+1 == argc) cmd[argc] = NULL;
		    else cmd[i] = argv[i+1];
		}
	//verificar se $n aparece, se sim, remover $ e guardar a sua respectiva coluna 
	for(i=0;i<total;i++) {
		tmp = cmd[i]; 
		if (*tmp == '$'){ // se começa por $
			tmp++;    // avanço um caracter e fico a apontar para o primeiro dígito
			coluna = atoi(tmp); //número da coluna armazenado em coluna.
			achei = 1;
			break; //sair do ciclo ao achar
		}
		coluna++;
	}

	//processar input
	while((n = read(0,buffer,PIPE_BUF))) {      
    	//Achar a(s) coluna(s)
    	char *ptr = buffer;
    	cut = 0;
    	while ( sscanf(ptr, "%[^:]%n", field, &s) == 1) {
        	cut++;
        	if(cut == coluna) sprintf(print,"%s",field); //achou a coluna, guardar valor no print
        	ptr += s; // avançar os characteres lidos
         	++ptr; // salta o : 
      	}
		//mudar comando a executar com valor da coluna já convertido, caso haja algum $n
    	if(achei) cmd[coluna] = print; 
    	//faz fork e o filho executa o comando com os valores alterados
      	pid = fork(); //guardar pid filho
      	if(pid==0) { 
      		int devNull = open("/dev/null", O_WRONLY); 
      		dup2(devNull,1); //mandar output para /dev/null
      		dup2(devNull,2); //stderr putput para /dev/null
      		execvp(cmd[0],cmd); 
      	}
		//pai faz waitpid e guarda exit status
     	waitpid(pid,&status,0);
     	buffer[n-1] = '\0'; //tirar /n
      	if(WIFEXITED(status)) { sprintf(final,"%s:%i\n",buffer,WEXITSTATUS(status)); } //adicionar o exit status
		write(1,final,strlen(final));
	}

  return 0; //nunca aqui vai chegar, mas é menos um warning ao compilar

}