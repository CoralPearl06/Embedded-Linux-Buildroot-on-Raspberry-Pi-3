.RECIPEPREFIX := >
CC = gcc
all: server receiver

server: server.c
> $(CC) server.c -o server -lwebsockets -lrt

receiver: receiver.c
> $(CC) receiver.c -o receiver -lrt

clean:
> rm -f server receiver
