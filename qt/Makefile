CPPFLAGS := `pkg-config --cflags Qt5Widgets` -g -std=c++17 -fpic
LFLAGS := `pkg-config --libs Qt5Widgets` -L../client -lchat
REMOVE := rm
COMPILER := g++
EXECUTABLE := chatqt

OBJECTS := main.o Log.cc Dialog.o Session.o
.PHONY := clean all

all: $(EXECUTABLE)
	LD_LIBRARY_PATH=../client ./$(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(COMPILER) -o $@ $(LFLAGS) $(OBJECTS)

%.o: %.cc *.h
	$(COMPILER) -c $(CPPFLAGS) $<

clean:
	$(REMOVE) $(OBJECTS)