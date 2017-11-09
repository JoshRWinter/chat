echo BUILDING THE SERVER
cd server
cl /EHsc /std:c++17 *.cc ..\sqlite3.lib ws2_32.lib /link /out:chat-server.exe
cd ..

echo BUILDING THE CLIENT
cd client
cl /EHsc /std:c++17 *.cc ..\sqlite3.lib ws2_32.lib /link /dll /out:chat.dll
dumpbin /exports chat.dll > chat.exports
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
cl /I"C:\Qt\5.9.1\msvc2017_64\include" /I"C:\Qt\5.9.1\msvc2017_64\include\QtCore" /I"C:\Qt\5.9.1\msvc2017_64\include\QtGui" /I"C:\Qt\5.9.1\msvc2017_64\include\QtWidgets" /EHsc /std:c++17 *.cc chat.lib C:\Qt\5.9.1\msvc2017_64\lib\Qt5Core.lib C:\Qt\5.9.1\msvc2017_64\lib\Qt5Widgets.lib C:\Qt\5.9.1\msvc2017_64\lib\Qt5Gui.lib /link /out:chatqt.exe
cd ..
