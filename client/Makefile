CPPFLAGS := -c -O2 -std=c++14 -fpic -Wall -pedantic
LFLAGS := -lsqlite3 -pthread -shared -lsqlite3 -s
COMPILER := g++
REMOVE := rm

OBJECTS := network.o ChatClient.o ChatService.o Database.o log.o lite3.o

libchat.so: $(OBJECTS)
	$(COMPILER) -o $@ $(LFLAGS) $(OBJECTS)

%.o: %.cc *.h ../chat.h
	$(COMPILER) $(CPPFLAGS) $<

.PHONY: clean
clean:
	$(REMOVE) *.o
