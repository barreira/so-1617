#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "readln.h"

/*
 * @brief Este programa reproduz as linhas que satisfazem uma condicão (=, >=,
 * <=, >, <, !=) indicada nos seus argumentos.
 *
 * filter <coluna> <operador> <operando>
 *
 * Utilização: ./a.out coluna "condição" valor-de-comparação
 *
 * filter 2 "<=" 10
 * input: a:5:c 
 * output: a:5:c
 *
 * filter 2 ">" 20
 * input a:10:c 
 * output:
 */
int main(int argc, char const *argv[]) {
    char buffer[PIPE_BUF], print[PIPE_BUF], final[PIPE_BUF], field[100];
    int n, coluna, valor, s, cut;

    coluna = atoi(argv[1]);
    valor = atoi(argv[3]);

    while ((n = readln(0, buffer, PIPE_BUF)) >= 0) { // não pára com EOF 
        if (n != 0) {

            // Achar a coluna
            
            char *ptr = buffer;
            cut = 0;
        
            while (sscanf(ptr, "%100[^:]%n", field, &s) == 1) {
                cut++;
                if (cut == coluna) {
                    sprintf(print, "%s", field); // achou a coluna, guardar
                                                 // valor no print
                }
                ptr += s; // avançar os carateres lidos
                ++ptr; // salta o :
            }

            //verifica o argumento e faz a comparação
            sprintf(final, "%s\n", buffer);
            if (strcmp(argv[2], "=") == 0)  if (atoi(print) == valor) write(1, final, strlen(final));
            if (strcmp(argv[2], ">=") == 0) if (atoi(print) >= valor) write(1, final, strlen(final));
            if (strcmp(argv[2], "<=") == 0) if (atoi(print) <= valor) write(1, final, strlen(final));
            if (strcmp(argv[2], ">") == 0)  if (atoi(print) > valor) write(1, final, strlen(final));
            if (strcmp(argv[2], "<") == 0)  if (atoi(print) < valor) write(1, final, strlen(final));
            if (strcmp(argv[2], "!=") == 0) if (atoi(print) != valor) write(1, final, strlen(final));
        }
    }

    return 0;
}