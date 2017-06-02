#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "readln.h"

/*window <coluna> <operacao> <linhas>
Este programa reproduz todas as linhas acrescentando-lhe uma nova coluna com o resultado de uma
operacão  é calculada sobre os valores da coluna indicada nas linhas anteriores.
avg, max, min, sum

./a.out 1 sum 3

input: 10:a:b
output: 10:a:b:10

input: 10
output: 10:20

input: 5:c:d
output: 5:c:d:25
*/


int main(int argc, char const *argv[]){

	int linhas = atoi(argv[3]);
	int coluna = atoi(argv[1]);
	int stored[linhas]; //armazena valores
	int cut,n,i,res,first=0,s;
	char buffer[PIPE_BUF];
	char print[PIPE_BUF];
	char final[PIPE_BUF];
	char field[10];
	
	//AVG
	int do_avg(){
		if(first == 0) { first++; return 0; } //quando começa dá sempre 0
		if(first == 1) {first++; return stored[0]; } //quando só tem uma coluna dá o valor dessa coluna
		else if(first < linhas) { //a partir dai enquanto que não houver buffer, utiliza as x linhas existentes
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
		if(first == 0) { first++;	for(i=0;i<linhas;i++) { stored[i] = res; } } //incialização para máximo
		int max;
		max = stored[0];
		for(i=1;i<linhas;i++) {
			if(max < stored[i]) max = stored[i];
		}
		return max;
	}
	//min
	int do_min(){
		if(first == 0) { first++;	for(i=0;i<linhas;i++) { stored[i] = res; } } //incialização para min
		int min;
		min = stored[0];
		for(i=1;i<linhas;i++) {
			if(min > stored[i]) min = stored[i];
		}
		return min;
	}
	//SUM
	int do_sum(){
		if(first == 0)  { for(i=1;i<linhas;i++) { stored[i] = 0; } first++; } //inicializar todos os valores a 0
		int sum = 0;
		for(i=0;i<linhas;i++) {
			sum = sum + stored[i];
		}
		return sum;
	}

	//faz operação
	void do_op() {
		if(strcmp(argv[2],"avg") == 0) res = do_avg();
		if(strcmp(argv[2],"max") == 0) res = do_max();
		if(strcmp(argv[2],"min") == 0) res = do_min();
		if(strcmp(argv[2],"sum") == 0) res = do_sum();
	}

	//adiciona um novo valor ao array e tira o mais antigo
	void novo_valor(int a){
		for(i=linhas;i>=1;i--) {
			stored[i] = stored[i-1];
		}
		stored[0] = a;
	}

   while((n = readln(0,buffer,PIPE_BUF)) >= 0) {  
      if(n!=0) {  
         
      //Achar a coluna
      char *ptr = buffer;
      cut = 0;
      while ( sscanf(ptr, "%10[^:]%n", field, &s) == 1) {
         cut++;
         if(cut == coluna) sprintf(print,"%s\n",field); //achou a coluna, guardar valor no print
         ptr += s; /* avançar os characteres lidos */
         ++ptr; /* salta o : */
      }
      //fazer as operações
      res = atoi(print); //guardar valor para inteiro
      novo_valor(res); //adiciona novo valor
      do_op(); //faz as contas e actualiza res(ultado)
      //buffer[n-1] = '\0'; //tirar /n
	  sprintf(final,"%s:%i\n",buffer,res); //acrescentar resultado fim da linha
	  write(1,final,strlen(final));
	}

  }

  return 0; //nunca aqui vai chegar, mas é menos um warning ao compilar
}