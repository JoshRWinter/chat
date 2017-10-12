#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

#include <signal.h>

#include "../chat.h"
#include "Server.h"
#include "log.h"

static std::atomic<bool> running;
void handler(int);

int main(int argc, char **argv){
	running.store(true);
	signal(SIGINT,handler);
	signal(SIGTERM,handler);
	signal(SIGPIPE,handler);

	{
		const int port=MCHAT_PORT;
		Server server(port);
		if(!server){
			std::cerr<<"error: could not bind to port "<<MCHAT_PORT<<"!"<<std::endl;
			if(port<=1024)
				std::cerr<<"also make sure you have permission to bind to this port."<<std::endl;
			return 1;
		}

		// status line
		std::cout<<"[ready on tcp:"<<port<<"]"<<std::endl;

		while(running.load()){
			server.accept();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

	std::cout<<"exiting..."<<std::endl;

	return 0;
}

void handler(int sig){
	switch(sig){
	case SIGTERM:
	case SIGINT:
		running.store(false);
		log("");
		break;
	case SIGPIPE:
		break;
	}
}
