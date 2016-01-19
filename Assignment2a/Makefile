CC=gcc
CFLAGS=-c -Werror 
LIBS=./

all: router pktgen

router: router.o
	$(CC)  -o router $^

pktgen: pktgen.o
	$(CC)  -o pktgen $^ 
	
%.o: %.c 
	$(CC) $(CFLAGS) -I$(LIBS) -c -o $@ $<


clean:
	rm -rf *.o router pktgen
