#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>


/*filter <coluna> <operador> <operando>
Este programa reproduz as linhas que satisfazem uma condicão indicada nos seus argumentos. 
=, >=, <=, >, <, !=.

./a.out coluna "condição" valor-de-comparação

input: a:5:c "<=" 10
output: a:5:c

input a:10:c ">" 20
output:

*/


int main(int argc, char const *argv[]){

   char buffer[PIPE_BUF];
   char print[PIPE_BUF];
   int n, coluna = atoi(argv[1]), valor = atoi(argv[3]),s,cut;
   char field[10];
   
   while(n = read(0,buffer,PIPE_BUF)) {      
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
   //verifica o argumento e faz a comparação
   if(strcmp(argv[2],"=") == 0) if(atoi(print) == valor) { write(1,buffer,n);  } 
   if(strcmp(argv[2],">=") == 0) if(atoi(print) >= valor){ write(1,buffer,n); } 
   if(strcmp(argv[2],"<=") == 0) if(atoi(print) <= valor) { write(1,buffer,n); }
   if(strcmp(argv[2],">") == 0) if(atoi(print) > valor) { write(1,buffer,n); } 
   if(strcmp(argv[2],"<") == 0) if(atoi(print) < valor) { write(1,buffer,n); }
   if(strcmp(argv[2],"!=") == 0) if(atoi(print) != valor) { write(1,buffer,n); }
   }


}