win:
	gcc -g -D _WIN32 main.c socket.c -std=c99 -lws2_32 -o simpleIRCd

linux:
	gcc -g -D linux main.c socket.c -std=c99 -o simpleIRCd