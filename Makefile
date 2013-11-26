all: client server

client: client.c number.h number.c nullstring.h nullstring.c nsocket.h nsocket.c
	gcc -o client -g client.c number.h number.c nullstring.h nullstring.c nsocket.h nsocket.c -lev

server: server.c number.h number.c nullstring.h nullstring.c nsocket.h nsocket.c linklist.h linklist.c shmformat.h shmformat.c
	gcc -o server -g server.c number.h number.c nullstring.h nullstring.c nsocket.h nsocket.c linklist.h linklist.c shmformat.h shmformat.c -lev

clean:
	rm client server
