CPPFLAGS := `pkg-config --cflags Qt5Widgets` -O2 -std=c++17 -fpic -Wall -pedantic
LFLAGS := `pkg-config --libs Qt5Widgets` -L../client -lchat -s
REMOVE := rm
COMPILER := g++
EXECUTABLE := chatqt

OBJECTS := main.o Dialog.o Session.o MessageThread.o
.PHONY := clean all

all: $(EXECUTABLE)
	LD_LIBRARY_PATH=../client ./$(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(COMPILER) -o $@ $(LFLAGS) $(OBJECTS)

%.o: %.cc *.h
	$(COMPILER) -c $(CPPFLAGS) $<

install:
	cp ../client/libchat.so /usr/lib
	cp chatqt /usr/bin
	cp ../server/chat-server /usr/bin
	cp icon.png /usr/share/icons/hicolor/128x128/apps/chatqt.png
	cp chatqt.desktop /usr/share/applications
	echo successfully installed

uninstall:
	rm -f /usr/lib/libchat.so
	rm -f /usr/bin/chatqt
	rm -f /usr/bin/chat-server
	rm -f /usr/share/icons/hicolor/128x128/apps/chatqt.png
	rm -f /usr/share/applications/chatqt.desktop
	echo successfully uninstalled

clean:
	$(REMOVE) $(OBJECTS)
