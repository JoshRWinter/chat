#include <iostream>
#include <cstdint>
#include <atomic>
#include <thread>
#include <chrono>

#include <sys/signal.h>

#include "../network.h"
#include "../mchat.h"

std::atomic<bool> running;

void handler(int sig){
	switch(sig){
	case SIGTERM:
	case SIGINT:
		running.store(false);
		break;
	}
}

int main(){
	char name[]="Josh Winter";
	running.store(true);

	signal(SIGINT,handler);
	signal(SIGTERM,handler);

	net::tcp tcp("localhost",MCHAT_PORT);
	if(!tcp.connect(5)){
		std::cerr<<"error: could not connect to localhost"<<std::endl;
		return 1;
	}

	std::uint32_t size=sizeof(name);
	tcp.send_block(&size,sizeof(size));
	if(tcp.error()){
		std::cerr<<"error"<<std::endl;
		return 1;
	}

	tcp.send_block(name,size);
	if(tcp.error()){
		std::cerr<<"error"<<std::endl;
		return 1;
	}

	while(running.load())
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	std::cout<<"exiting..."<<std::endl;

	return 0;
}
