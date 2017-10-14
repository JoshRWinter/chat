CPPFLAGS := -c -g -std=c++11 -fpic
LFLAGS := -lsqlite3 -pthread -shared
COMPILER := g++
REMOVE := rm

OBJECTS := network.o ChatClient.o ChatService.o ChatWorkUnit.o log.o

interface: interface.cc libchat.so ChatClient.h
	g++ -o $@ $< -L. -lchat

libchat.so: $(OBJECTS)
	$(COMPILER) -o $@ $(LFLAGS) $(OBJECTS)

%.o: %.cc *.h
	$(COMPILER) $(CPPFLAGS) $<

.PHONY: clean
clean:
	$(REMOVE) *.o
