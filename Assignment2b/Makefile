CC=gcc
CFLAGS=-c -Werror 
LIBS=./

all: file_server file_client

file_server: server.o
	$(CC)  -o file_server $^

file_client: client.o
	$(CC)  -o file_client $^ 
	
%.o: %.c 
	$(CC) $(CFLAGS) -I$(LIBS) -c -o $@ $<


clean:
	rm -rf *.o file_server file_client
