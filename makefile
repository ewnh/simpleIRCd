CC = gcc
CFLAGS = -g -march=native -O2 -Wall -Wextra -std=c99 -flto

ifeq ($(OS),Windows_NT)
	CFLAGS += -D_WIN32
	LDFLAGS = -lws2_32
	RM = del simpleIRCd.exe
else
	CFLAGS += -Dlinux -D_XOPEN_SOURCE=500
	LDFLAGS = -pthread
	RM = rm simpleIRCd
endif

all: main.o socket.o parser.o commands.o helpers.o
	$(CC) $(CFLAGS) main.o socket.o parser.o commands.o helpers.o $(LDFLAGS) -o simpleIRCd

main.o: src/main.c src/socket.h src/defines.h
	$(CC) $(CFLAGS) -c src/main.c

socket.o: src/socket.c src/socket.h src/helpers.h
	$(CC) $(CFLAGS) -c src/socket.c
    
parser.o: src/parser.c src/socket.h src/commands.h src/helpers.h src/defines.h
	$(CC) $(CFLAGS) -c src/parser.c
    
commands.o: src/commands.c src/commands.h src/defines.h src/helpers.h src/socket.h
	$(CC) $(CFLAGS) -c src/commands.c
    
helpers.o: src/helpers.c src/defines.h
	$(CC) $(CFLAGS) -c src/helpers.c

install: all
	sudo cp simpleIRCd /usr/local/bin

clean:
	$(RM) *.o
