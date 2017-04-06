win:
	gcc -g -O2 -D _WIN32 src/main.c src/socket.c src/parser.c src/commands.c -std=c99 -lws2_32 -o simpleIRCd

linux:
	gcc -g -O2 -D linux src/main.c src/socket.c src/parser.c src/commands.c -std=c99 -pthread -o simpleIRCd
