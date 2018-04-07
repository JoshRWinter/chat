#include "os.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif // _WIN32

void os::mkdir(const std::string &dir){
#ifdef _WIN32
	_mkdir(dir.c_str());
#else
	::mkdir(dir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
#endif // _WIN32
}
