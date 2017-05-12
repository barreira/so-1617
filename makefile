CC = gcc
CFLAGS = -Wall -g
OUT = controlador
OBJS = controlador.o

controlador: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(OUT)

controlador.o: controlador.c
	cc -c controlador.c

clean:
	rm -f *.o controlador
