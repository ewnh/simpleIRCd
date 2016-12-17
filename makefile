win:
	gcc -g -D _WIN32 src/main.c src/socket.c src/parser.c -std=c99 -lws2_32 -o simpleIRCd

linux:
	gcc -g -D linux src/main.c src/socket.c src/parser.c -std=c99 -pthread -o simpleIRCd
