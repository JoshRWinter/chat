CPPFLAGS := -g -c -std=c++14
LFLAGS := -pthread -lsqlite3
COMPILER := g++
REMOVE := rm

OBJECTS := ../network.o log.o main.o Server.o Client.o Command.o Database.o

chat-server: $(OBJECTS)
	$(COMPILER) -o $@ $(OBJECTS) $(LFLAGS)

%.o: %.cc *.h
	$(COMPILER) $(CPPFLAGS) $<

.PHONY: clean
clean:
	$(REMOVE) *.o
