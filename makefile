all:
	gcc main.c -std=c99 -lws2_32 -o simpleIRCd
