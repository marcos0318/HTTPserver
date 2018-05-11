all: server client

server: pthread_server.c
	gcc pthread_server.c -lpthread -o server

client: client.c
	gcc client.c -o client
