call qtpath.bat

echo BUILDING THE SERVER
cd server
cl /EHsc /std:c++17 *.cc ..\sqlite3.lib ws2_32.lib /link /out:chat-server.exe
cd ..

echo BUILDING THE CLIENT
cd client
cl /EHsc /std:c++17 *.cc ..\sqlite3.lib ws2_32.lib /link /dll /out:chat.dll
dumpbin /exports chat.dll > chat.exports
echo LIBRARY CHAT > chat.def
echo EXPORTS >> chat.def
for /f "skip=19 tokens=4" %%A in (chat.exports) do echo %%A >> chat.def
lib /def:chat.def /out:chat.lib /machine:x64
del chat.exports
del chat.exp
del chat.def
move chat.lib ..\qt
move chat.dll ..\qt
cd ..

echo BUILDING QT FRONT END
cd qt
cl /I%qtpath%\include /I%qtpath%\include\QtCore /I%qtpath%\include\QtGui /I%qtpath%\include\QtWidgets /EHsc /std:c++17 *.cc chat.lib %qtpath%\lib\Qt5Core.lib %qtpath%\lib\Qt5Widgets.lib %qtpath%\lib\Qt5Gui.lib /link /out:chatqt.exe
cd ..
