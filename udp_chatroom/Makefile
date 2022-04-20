all:server client

server:server.o

client:client.o

%*.o:%*.c

.PHONY:clean

clean:
	$(RM) *.o a.out core server client
