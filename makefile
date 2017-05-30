CC = gcc
CFLAGS = -Wall -g
#OUT = controlador const filter window spawn
#OBJS = controlador.c const.c filter.c window.c spawn.c

all:
	rm -rf tmp
	mkdir tmp
	$(CC) const.c $(CFLAGS) -o const
	$(CC) filter.c $(CFLAGS) -o filter
	$(CC) window.c $(CFLAGS) -o window
	$(CC) spawn.c $(CFLAGS) -o spawn
	$(CC) controlador.c $(CFLAGS) -o controlador

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

