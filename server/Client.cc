#include <functional>
#include <cstdint>
#include <iostream>

#include "Client.h"
#include "Command.h"
#include "log.h"

Client::Client(const Server &p,int sockfd):parent(p),tcp(sockfd),thread(std::ref(*this)){
	disconnected.store(false);
	last_heartbeat=0;
}

std::thread &Client::get_thread(){
	return thread;
}

const std::string &Client::get_name()const{
	return name;
}

bool Client::dead()const{
	return disconnected;
}

void Client::operator()(){
	try{
		get_info();
		loop();
	}catch(const NetworkException &e){
		log(name + " has disconnected");
	}catch(const ShutdownException &e){
		log(std::string("kicking ") + name);
	}
}

void Client::send(const void *data,unsigned size){
	unsigned sent=0;
	while(sent!=size){
		sent+=tcp.send_nonblock((char*)data+sent,size-sent);

		if(!parent.running())
			throw ShutdownException();
		else if(tcp.error())
			throw NetworkException();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Client::recv(void *data,unsigned size){
	int got=0;
	while(got!=size){
		got+=tcp.recv_nonblock((char*)data+got,size-got);

		if(!parent.running())
			throw ShutdownException();
		else if(tcp.error())
			throw NetworkException();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Client::get_info(){
	// get the name length
	std::uint32_t size;
	recv(&size,sizeof(size));

	//get the name
	std::vector<char> data(size+1);
	recv(&data[0],size);
	data[size]=0;
	name=&data[0];

	log(name + " has joined the chat");
}

void Client::loop(){
	for(;;){
		if(!parent.running())
			throw ShutdownException();

		Client::heartbeat();
	}
}

void Client::heartbeat(){
	time_t current=time(NULL);

	if(current<last_heartbeat) // user messed with system clock
		last_heartbeat=0;

	if(current-last_heartbeat>HEARTBEAT_FREQUENCY)
		Command::heartbeat(*this);
}

void Client::disconnect(){
	disconnected.store(true);
}
