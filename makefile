CC = gcc
CFLAGS = -Wall -g

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
	rm -f *.o controlador const filter window spawn