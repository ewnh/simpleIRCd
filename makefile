all:
	gcc main.c socket.c -std=c99 -lws2_32 -o simpleIRCd
