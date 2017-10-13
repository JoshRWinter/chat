#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <exception>

#include <signal.h>

#include "../chat.h"
#include "Server.h"
#include "log.h"

struct config{
	unsigned short port;
	std::string dbname;
};

static std::atomic<bool> running;
void handler(int);
void go(const config&);

int main(int argc, char **argv){
	running.store(true);

	// signal handlers
	signal(SIGINT,handler);
	signal(SIGTERM,handler);
	signal(SIGPIPE,handler);

	// parameters
	config cfg;
	cfg.port=CHAT_PORT;
	cfg.dbname="chatdb";

	try{
		go(cfg);
	}catch(const std::exception &e){
		std::cerr<<"\033[31;1mfatal error:\033[0m "<<e.what()<<std::endl;
	}

	std::cout<<"exiting..."<<std::endl;

	return 0;
}

void go(const config &cfg){
	Server server(cfg.port,cfg.dbname);

	// status line
	std::cout<<"[ready on tcp:"<<cfg.port<<"]"<<std::endl;

	while(running.load()){
		server.accept();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
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
