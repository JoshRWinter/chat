#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <exception>

#include <string.h>

#include "../chat.h"
#include "Server.h"
#include "log.h"

#ifdef _WIN32
#include <windows.h>
BOOL WINAPI handler(DWORD);
#else
#include <signal.h>
static void handler(int);
#endif // _WIN32

struct config{
	unsigned short port;
	std::string dbname;
};

static std::atomic<bool> running;
static std::string getdbpath();
static void go(const config&);

int main(int argc, char **argv){
	running.store(true);

#ifdef _WIN32
	if(!SetConsoleCtrlHandler(handler, TRUE)){
		std::cout<<"error setting console handler"<<std::endl;
		return 1;
	}
#else
	// signal handlers
	signal(SIGINT,handler);
	signal(SIGTERM,handler);
	signal(SIGPIPE,handler);
#endif // _WIN32

	// parameters
	config cfg;
	cfg.port=CHAT_PORT;
	cfg.dbname=argc>1?argv[1]:getdbpath();

	try{
		go(cfg);
	}catch(const std::exception &e){
		std::cerr<<"\033[31;1mfatal error:\033[0m "<<e.what()<<std::endl;
	}

	std::cout<<"exiting..."<<std::endl;

#ifdef _WIN32
	Sleep(700);
#endif // _WIN32

	return 0;
}

#ifdef _WIN32
std::string getdbpath(){
	const char *path="%userprofile%\\chat-server.db";
	char expanded[150]="INVALID PATH";

	ExpandEnvironmentStrings(path, expanded, 149);

	return expanded;
}
#else
#include <wordexp.h>
std::string getdbpath(){
	const char *path="~/.chat-server-db";
	wordexp_t p;
	wordexp(path, &p, 0);

	std::string fqpath=p.we_wordv[0];

	wordfree(&p);
	return fqpath;
}
#endif // _WIN32

void go(const config &cfg){
	Server server(cfg.port,cfg.dbname);

	// status line
	std::cout<<"[ready on tcp:"<<cfg.port<<"]"<<std::endl;

	while(running.load()){
		server.accept();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

#ifdef _WIN32
BOOL WINAPI handler(DWORD signal){
	if(signal==CTRL_C_EVENT){
		running.store(false);
	}

	return true;
}
#else
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
#endif // _WIN32
