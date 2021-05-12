all: server client

server: server.c server.h list_fd.c list_fd.h
	gcc -o server server.c server.h list_fd.c list_fd.h -pthread -g

client: client.c
	gcc -o client client.c

clean:
	rm -f server client
