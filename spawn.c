#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>

#include "readln.h"

/*
 * @brief Este programa reproduz todas as linhas, executando o comando indicado
 * uma vez para cada uma delas, e acrescentando uma nova coluna com o respetivo
 * exit status.
 * 
 *        e.g. spawn mailx -s $n x@y.com
 * 			   (substituir $n pelo valor da coluna)
 *
 * Utilização: ./a.out mailx -s \$3 x@y.com
 */
int main(int argc, char const *argv[]) {

	int coluna = 0, achei = 0; //guardar o $n
	int total, i, n, cut, s, pid, status;
	char buffer[PIPE_BUF], print[PIPE_BUF], final[PIPE_BUF], field[PIPE_BUF];
	char *tmp;
	char *cmd[total]; //guarda o comando a executar

	total = argc-1;

	for (i = 0; i < argc; i++) {
		    if (i+1 == argc) cmd[argc] = NULL;
		    else cmd[i] = argv[i+1];
	}

	// verificar se $n aparece, se sim, remover $ e guardar a sua respectiva coluna 
	for (i = 0; i < total; i++) {
		tmp = cmd[i]; 
		if (*tmp == '$') { // se começa por $
			tmp++;    // avanço um caracter e fico a apontar para o primeiro dígito
			coluna = atoi(tmp); // número da coluna armazenado em coluna.
			achei = 1;
			break; // sair do ciclo ao achar
		}
		coluna++;
	}

	//processar input
    while ((n = readln(0, buffer, PIPE_BUF)) >= 0) {  
        if (n != 0) {   

    	    //Achar a(s) coluna(s)
    	    
			char *ptr = buffer;
    	    cut = 0;
    	    
			while (sscanf(ptr, "%[^:]%n", field, &s) == 1) {
        	    cut++;
        	    if (cut == coluna) sprintf(print, "%s", field); // achou a coluna, guardar valor no print
        	    ptr += s; // avançar os characteres lidos
         	    ++ptr; // salta o : 
      	    }

		    // mudar comando a executar com valor da coluna já convertido, caso haja algum $n
    	    
			if (achei) cmd[coluna] = print; 
    	    
			// faz fork e o filho executa o comando com os valores alterados
      	    
			pid = fork(); //guardar pid filho
      	    
			  if (pid == 0) { 
      		    int devNull = open("/dev/null", O_WRONLY); 
      		    dup2(devNull, 1); //mandar output para /dev/null
      		    dup2(devNull, 2); //stderr putput para /dev/null
      		    execvp(cmd[0], cmd); 
      	    }

		    // pai faz waitpid e guarda exit status
     	    waitpid(pid, &status, 0);

      	    if (WIFEXITED(status)) sprintf(final, "%s:%i\n", buffer, WEXITSTATUS(status)); // adicionar o exit status
		    write(1, final, strlen(final));
	    }
 	}

    return 0;
}