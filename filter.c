#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>


/*filter <coluna> <operador> <operando>
Este programa reproduz as linhas que satisfazem uma condic ̧ a  ̃ o indicada nos seus argumentos. 
=, >=, <=, >, <, !=.
*/


int main(int argc, char const *argv[]){

   char buffer[PIPE_BUF];
   char print[PIPE_BUF];
   int n, coluna = atoi(argv[1]), valor = atoi(argv[3]);
   char field[10];
   int s,cut;
   
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
   //write(1,print,strlen(print));
   //verifica o argumento e faz a comparação
   if(strcmp(argv[2],"=") == 0) if(atoi(print) == valor) { write(1,buffer,n);  } //printf("ACERTEI NO =\n"); 
   if(strcmp(argv[2],">=") == 0) if(atoi(print) >= valor){ write(1,buffer,n); } //printf("ACERTEI NO >=\n"); }
   if(strcmp(argv[2],"<=") == 0) if(atoi(print) <= valor) { write(1,buffer,n); } //printf("ACERTEI NO <=\n"); }
   if(strcmp(argv[2],">") == 0) if(atoi(print) > valor) { write(1,buffer,n); } //printf("ACERTEI NO >\n"); }
   if(strcmp(argv[2],"<") == 0) if(atoi(print) < valor) { write(1,buffer,n); } //printf("ACERTEI NO <\n"); }
   if(strcmp(argv[2],"!=") == 0) if(atoi(print) != valor) { write(1,buffer,n); } //printf("ACERTEI NO !=\n"); }

   }


}