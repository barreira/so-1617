#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>


/* Este programa reproduz as linhas acrescentando uma nova coluna sempre com o mesmo valor: 
utilização ./a.out const
input: a:b:c
output: a:b:c:const
*/

int main(int argc, char const *argv[]){

	

	char buffer[PIPE_BUF];
	char print[PIPE_BUF];
	int n;

	while((n = read(0,buffer,PIPE_BUF)) >= 0) {	
		if(n!=0) {
		buffer[n-1] = '\0'; //tirar /n
		sprintf(print,"%s:%s\n",buffer,argv[1]); //acrescentar resto :const
		write(1,print,strlen(print)); //write stdout
		}
		//else { pause(); }
	}

  return 0; //nunca aqui vai chegar, mas é menos um warning ao compilar

}