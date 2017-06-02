#ifndef READLN_H
#define READLN_H

/*
 * @brief Lê uma linha
 *
 * @param fildes Descritor de ficheiro de onde se lê
 * @param buf    Buffer para onde se escreve os dados lidos
 * @param nbyte  Número máximo de bytes a ler
 *
 * @return Retorna o número de bytes lidos
 */
ssize_t readln(int fildes, void* buf, size_t nbyte) {
	int i, n;

	for (i = 0; i < nbyte; i++) {
		n = read(fildes, buf+i, 1);

		if (n == -1) return -1;

		if (n == 0) {
            break;
        }

		if (((char*) buf)[i] == '\n') {
            break;
        }
	}

    ((char*) buf)[i] = '\0';

	return i;
}

#endif