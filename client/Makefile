CPPFLAGS := -c -g -std=c++11 -fpic -Wall -pedantic
LFLAGS := -lsqlite3 -pthread -shared -lsqlite3
COMPILER := g++
REMOVE := rm

OBJECTS := network.o ChatClient.o ChatService.o Database.o log.o

interface: interface.cc libchat.so ChatClient.h
	g++ -o $@ -std=c++17 -g $< -L. -lchat

libchat.so: $(OBJECTS)
	$(COMPILER) -o $@ $(LFLAGS) $(OBJECTS)

%.o: %.cc *.h ../chat.h
	$(COMPILER) $(CPPFLAGS) $<

.PHONY: clean
clean:
	$(REMOVE) *.o
