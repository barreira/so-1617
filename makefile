#CC = gcc
#CFLAGS = -Wall -g
#OUT = controlador
#OBJS = controlador.o
#
#all:
#
#controlador: $(OBJS)
#	$(CC) $(OBJS) $(CFLAGS) -o $(OUT)
#
#controlador.o: controlador.c
#	cc -c controlador.c
#
#clean:
#	rm -f *.o controlador

CC=gcc
CFLAGS= -Wall -g
SOURCES= const.c filter.c window.c spawn.c controlador.c
OBJECTS=$(SOURCES:.c=.o)

all: 
	mkdir tmp #onde os fifos ficam guardados
	$(CC) $(CFLAGS) $(SOURCES) -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
   
clean:
	rm -f *.o controlador const window spawn filter
	rm -rf ./tmp

