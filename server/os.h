#ifndef CHAT_OS_H
#define CHAT_OS_H

#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif // _WIN32

namespace os{
	void mkdir(const std::string&);
}

#endif // CHAT_OS_H
