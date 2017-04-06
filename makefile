win:
	gcc -g -O2 -D _WIN32 main.c socket.c -std=c99 -lws2_32 -o simpleIRCd

linux:
	gcc -g -O2 -D linux main.c socket.c -std=c99 -pthread -o simpleIRCd