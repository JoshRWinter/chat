#include <functional>
#include <cstdint>
#include <iostream>

#include "Client.h"
#include "Command.h"
#include "../chat.h"
#include "log.h"

Client::Client(const Server &p,int sockfd):
	parent(p),
	tcp(sockfd),
	disconnected(false),
	last_heartbeat(0),
	thread(std::ref(*this)) // start a separate event thread for this client (operator())
{}

std::thread &Client::get_thread(){
	return thread;
}

const std::string &Client::get_name()const{
	return name;
}

bool Client::dead()const{
	return disconnected.load();
}

// entry point for the client thread
void Client::operator()(){
	try{
		name=Command::recv_name(*this);
		loop();
	}catch(const NetworkException &e){
		log(name + " has disconnected");
	}catch(const ShutdownException &e){
		log(std::string("kicking ") + name);
	}
}

// send network data
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

// recv network data
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

// main processing loop for client
void Client::loop(){
	for(;;){
		if(!parent.running())
			throw ShutdownException();

		heartbeat();

		recv_command();
	}
}

// recv commands from the client
void Client::recv_command(){
	if(tcp.peek()<sizeof(ClientCommand))
		return;

	ClientCommand type;
	recv(&type,sizeof(type));

	switch(type){
	case ClientCommand::MESSAGE:
		log(Command::recv_message(*this));
		break;
	case ClientCommand::SUBSCRIBE:
		break;
	case ClientCommand::NEW_CHAT:
		Command::recv_newchat(*this);
		break;
	}
}

// send a heartbeat to the client
void Client::heartbeat(){
	time_t current=time(NULL);

	if(current<last_heartbeat) // user messed with system clock
		last_heartbeat=0;

	if(current-last_heartbeat>HEARTBEAT_FREQUENCY)
		Command::send_heartbeat(*this);
}

void Client::disconnect(){
	disconnected.store(true);
}
