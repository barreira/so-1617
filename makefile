CC = gcc
CFLAGS = -Wall -g
LIBS = `pkg-config --cflags --libs glib-2.0`
OUT = controlador
OBJS = controlador.o node.o

controlador: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(OUT) $(LIBS)

controlador.o: controlador.c node.h
	cc -c controlador.c $(LIBS)

node.o: node.c node.h
	cc -c node.c $(LIBS)

clean:
	rm -f *.o controlador
