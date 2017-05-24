CC = gcc
CFLAGS = -Wall -g
OUT = controlador
OBJS = controlador.o

controlador: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(OUT)

controlador.o: controlador.c
	cc -c controlador.c

const.o: const.c
	cc -c const.c

filter.o: filter.c
	cc -c filter.c

window.o: window.c
	cc -c window.c

spawn.o: spawn.c
	cc -c spawn.c		

clean:
	rm -rf tmp
	rm -f *.o controlador

#CC=gcc
#CFLAGS= -Wall -g
#SOURCES= const.c filter.c window.c spawn.c controlador.c
#OBJECTS=$(SOURCES:.c=.o)
#
#all: 
#	mkdir tmp #onde os fifos ficam guardados
#	$(CC) $(CFLAGS) $(SOURCES) -o $@
#
#%.o : %.c
#	$(CC) -c $(CFLAGS) $< -o $@
#   
#clean:
#	rm -f *.o controlador const window spawn filter
#	rm -rf ./tmp

