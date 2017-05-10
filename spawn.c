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

grep -Eo [\$1-9]{2} cmds.txt
*/

int main(int argc, char const *argv[]){

	int total = argc-1;
	int coluna =0; //guardar os $n
	int i,n,cut,s, pid,status;
	char *cmd[total]; //guarda o comando a executar
	//char *tmp;
	char buffer[PIPE_BUF];
	char print[PIPE_BUF];
	char final[PIPE_BUF];
	char col[10];
	char field[10];


	//passar argumentos para array ; remover ./a.out
	//--argc;
	//++argv;
	for(i=0;i<total;i++) {
		    if(i+1 == total) cmd[total+1] = NULL;
		    else cmd[i] = argv[i+1];
		}
	//verify for $n
	//char *tmp = cmd;

	//for(i=0;i<total;i++) {
	// 	if(strcmp(cmd[i][0],"$") ==0) { cmd[i][0] = cmd[i][1] ; cmd[i][1] = '\0'; coluna = atoi(cmd[i]);}
	//}

	//for(i=0;i<total;i++) { write(1,argv[i],strlen(argv[i])); }
	//guardar valor coluna a usar no argumento

	while(n = read(0,buffer,PIPE_BUF)) {      
    	buffer[n-1] = '\0'; //tirar /n
    	//Achar a coluna
    	/*
    	char *ptr = buffer;
    	cut = 0;
    	while ( sscanf(ptr, "%10[^:]%n", field, &s) == 1) {
        	cut++;
        	if(cut == coluna) sprintf(print,"%s",field); //achou a coluna, guardar valor no print
        	ptr += s; // avançar os characteres lidos
        	//if ( *ptr != ':' )  {  break; // falhou }
         	++ptr; // salta o : 
      	}
      	*/
		//trabalhar linha recebida
		//argv[coluna-1] = print;
    	//cmd[coluna-1] = print; //valor da coluna no comando a executar
    	//filho executar comando
      	pid = fork();
      	if(pid==0) { 
      		int devNull = open("/dev/null", O_WRONLY); 
      		dup2(devNull,1); //mandar output para /dev/null
      		dup2(devNull,2); //stderr putput para /dev/null
      		execvp(cmd[0],cmd); 
      	}
		//pai faz waitpid e guarda exit status
     	waitpid(pid,&status,0);
      	if(WIFEXITED(status)) { sprintf(final,"%s:%i\n",buffer,WEXITSTATUS(status)); }
		//print line
		write(1,final,strlen(final));
		//print command
		//for(i=0;i<total;i++) { write(1,cmd[i],strlen(cmd[i])); }
	}
}