ifeq ($(CC),)
	CC = $(CROSS_COMPILE)gcc
endif
ifeq ($(CFLAGS),)
	CFLAGS = -g -Wall -Werror
endif

all: server client
.DEFAULT_GOAL := server client

server: server.c
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDES)
	
client: client.c
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDES)

clean:
	rm -f *.o server client
