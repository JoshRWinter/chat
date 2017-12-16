CPPFLAGS := -c -g -std=c++17 -Wall -pedantic
LFLAGS := -pthread -lsqlite3
COMPILER := g++
REMOVE := rm -f

OBJECTS := network.o log.o main.o Server.o Client.o Database.o

chat-server: $(OBJECTS)
	$(COMPILER) -o $@ $(OBJECTS) $(LFLAGS)

%.o: %.cc *.h ../chat.h
	$(COMPILER) $(CPPFLAGS) $<

.PHONY: clean release
clean:
	$(REMOVE) *.o

release: clean
	$(COMPILER) -o chat-server -Wall -pedantic -O2 -std=c++17 *.cc $(LFLAGS) -s

