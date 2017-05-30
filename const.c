#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "readln.h"

/* Este programa reproduz as linhas acrescentando uma nova coluna sempre com o mesmo valor: 
utilização ./a.out const
input: a:b:c
output: a:b:c:const
*/

int main(int argc, char const *argv[]){

	

	char buffer[PIPE_BUF];
	char print[PIPE_BUF];
	char tmp[PIPE_BUF];
	char *aux;

	int n,i,t;

	while((n = read(0,buffer,PIPE_BUF)) >= 0) {	
		if(n!=0) {
			i=0;
			t=0;
			while(i<n) {
				if(buffer[i] != '\n') { tmp[t] = buffer[i] ; t++; }
				else {
					t++;
					tmp[t] = '\0';
					sprintf(print,"%s:%s\n",tmp,argv[1]); //acrescentar resto :const
					write(1,print,strlen(print)); //write stdout
					tmp[0] = '\0';
					t=0;
				}
				i++;
	    	}
	    }

	}

  return 0; //nunca aqui vai chegar, mas é menos um warning ao compilar

}