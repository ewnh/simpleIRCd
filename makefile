win:
	gcc -g -D_WIN32 src/main.c src/socket.c src/parser.c src/commands.c -std=c99 -lws2_32 -o simpleIRCd

linux:
	gcc -g -Dlinux -D_XOPEN_SOURCE=500 src/main.c src/socket.c src/parser.c src/commands.c -std=c99 -pthread -o simpleIRCd
