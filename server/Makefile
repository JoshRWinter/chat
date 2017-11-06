CPPFLAGS := -c -O2 -std=c++17 -Wall -pedantic
LFLAGS := -pthread -lsqlite3 -s
COMPILER := g++
REMOVE := rm

OBJECTS := network.o log.o main.o Server.o Client.o Database.o

chat-server: $(OBJECTS)
	$(COMPILER) -o $@ $(OBJECTS) $(LFLAGS)

%.o: %.cc *.h ../chat.h
	$(COMPILER) $(CPPFLAGS) $<

.PHONY: clean
clean:
	$(REMOVE) *.o
