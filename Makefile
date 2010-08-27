CC= gcc
LDFLAGS=
CFLAGS=

all: mifare_socket testprog

debug: CFLAGS=-DDEBUG_COMMANDS
debug: all

ldebug: CFLAGS=-DDEBUG_LOW_LEVEL
ldebug: all

obj/mifare_socket.o: src/mifare_socket.c
	$(CC) $(CFLAGS) -c -o $@ src/mifare_socket.c

obj/sl500.o: src/sl500.c
	$(CC) $(CFLAGS) -c -o $@ src/sl500.c

obj/testprog.o: src/testprog.c
	$(CC) $(CFLAGS) -c -o $@ src/testprog.c

bin/mifare_socket: obj/mifare_socket.o obj/sl500.o
	$(CC) $(LDFLAGS) -o $@ obj/mifare_socket.o obj/sl500.o

bin/testprog: obj/testprog.o obj/sl500.o
	$(CC) $(LDFLAGS) -o $@ obj/testprog.o obj/sl500.o

#Aliases
mifare_socket: bin/mifare_socket
testprog: bin/testprog

clean:
	rm -f obj/*.o bin/* src/*~ *~

