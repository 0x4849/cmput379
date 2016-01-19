CC=gcc
CFLAGS=-c -Werror 
LIBS=./

all: bankers

bankers: bankers.o
	$(CC)  -o bankers $^

%.o: %.c 
	$(CC) $(CFLAGS) -I$(LIBS) -c -o $@ $<

clean:
	rm -rf *.o bankers
