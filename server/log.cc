#include <iostream>
#include <mutex>

#include "log.h"

static std::mutex out_lock;

void log(const std::string &line){
	std::lock_guard<std::mutex> lock(out_lock);
	std::cout<<line<<std::endl;
}
