#include <mutex>
#include <iostream>

static std::mutex out_lock;

#define COLOR_ERROR "\033[33;1m"
#define COLOR_RESET "\033[0m"

void log(const std::string &line){
	std::lock_guard<std::mutex> lock(out_lock);

	std::cout<<line<<std::endl;
}

void log_error(const std::string &line){
	std::lock_guard<std::mutex> lock(out_lock);

	std::cout<<COLOR_ERROR<<line<<COLOR_RESET<<std::endl;
}
