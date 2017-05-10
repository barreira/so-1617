#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

/*window <coluna> <operacao> <linhas>
Este programa reproduz todas as linhas acrescentando-lhe uma nova coluna com o resultado de uma
operac ̧ a  ̃ o calculada sobre os valores da coluna indicada nas linhas anteriores.
avg, max, min, sum
*/

//só trabalha com inteiros
//avg dá inteiros


int main(int argc, char const *argv[]){

	int linhas = atoi(argv[3]);
	int coluna = atoi(argv[1]);
	int stored[linhas]; //armazena valores
	int cut,n,i,res,first=0,s;
	char buffer[PIPE_BUF];
	char print[PIPE_BUF];
	char final[PIPE_BUF];
	char field[10];

	//inicializar valores armazenados tudo a 0
	for(i=0;i<linhas;i++) { stored[i] = 0; }

	//AVG
	int do_avg(){
		if(first == 0) { first++; return 0; } //quando começa dá sempre 0
		if(first == 1) {first++; return stored[0]; } //quando só tem uma coluna dá o valor dessa coluna
		else if(first < linhas) {
					int avg = 0;
					for(i=0;i<first;i++) {
					avg = avg + stored[i];
					}
					avg = avg / first ;
					first++;
					return avg; }
		int avg = 0;
		for(i=0;i<linhas;i++) {
			avg = avg + stored[i];
		}
		avg = avg / linhas ;
		return avg;
	}
	//MAX
	int do_max(){
		if(first == 0) { first++;	for(i=0;i<linhas;i++) { stored[i] = res; } } //evitar MAX seja <0
		int max;
		max = stored[0];
		for(i=1;i<linhas;i++) {
			if(max < stored[i]) max = stored[i];
		}
		return max;
	}
	//min
	int do_min(){
		if(first == 0) { first++;	for(i=0;i<linhas;i++) { stored[i] = res; } } //evitar min >0
		int min;
		min = stored[0];
		for(i=1;i<linhas;i++) {
			if(min > stored[i]) min = stored[i];
		}
		return min;
	}
	//SUM
	int do_sum(){
		int sum = 0;
		for(i=0;i<linhas;i++) {
			sum = sum + stored[i];
		}
		return sum;
	}

	//faz operação
	int do_op() {
		if(strcmp(argv[2],"avg") == 0) return do_avg();
		if(strcmp(argv[2],"max") == 0) return do_max();
		if(strcmp(argv[2],"min") == 0) return do_min();
		if(strcmp(argv[2],"sum") == 0) return do_sum();
	}

	//adiciona um novo valor ao array
	void novo_valor(int a){
		for(i=linhas;i>=1;i--) {
			stored[i] = stored[i-1];
		}
		stored[0] = a;
	}

   while(n = read(0,buffer,PIPE_BUF)) {      
      //buffer[n-1] = '\0'; //tirar /n
      //Achar a coluna
      char *ptr = buffer;
      cut = 0;
      while ( sscanf(ptr, "%10[^:]%n", field, &s) == 1) {
         cut++;
         if(cut == coluna) sprintf(print,"%s\n",field); //achou a coluna, guardar valor no print
         ptr += s; /* avançar os characteres lidos */
         //if ( *ptr != ':' )  {  break; /* falhou*/  }
         ++ptr; /* salta o : */
      }
      //fazer operações
      res = atoi(print); //guardar valor
      novo_valor(res); //adiciona novo valor
      res = do_op(); //faz as contas
      buffer[n-1] = '\0'; //tirar /n
	  sprintf(final,"%s:%i\n",buffer,res); //acrescentar resultado fim da linha
	  write(1,final,strlen(final));

  }

}