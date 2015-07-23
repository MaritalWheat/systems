CC   = gcc
OPTS = -Wall -fPIC 

all: server client rpc libmfs

# this generates the target executables

rpc: rpc.o udp.o
	$(CC) -o rpc rpc.o udp.o 

server: server.o udp.o
	$(CC) -o server server.o udp.o 

client: client.o udp.o
	$(CC) -o client client.o udp.o

libmfs: libmfs.o udp.o
	$(CC) -fPIC -shared -o libmfs.so libmfs.o udp.o

# this is a generic rule for .o files 
%.o: %.c 
	$(CC) $(OPTS) -c $< -o $@

clean:
	rm -f rpc.o server.o udp.o client.o server client



