#include <functional>
#include <cstdint>
#include <iostream>

#include "Client.h"

Client::Client(const Server &p,int sockfd):parent(p),tcp(sockfd),thread(std::ref(*this)){
	disconnected.store(false);
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
	// get the name
	std::uint32_t size;
	if(!recv(&size,sizeof(size))){
		disconnect();
		return;
	}

	std::vector<char> data(size+1);
	if(!recv(&data[0],size)){
		disconnect();
		return;
	}
	data[size]=0;
	name=&data[0];

	std::cout<<"i have figured out their name is "<<name<<std::endl;

	disconnect();
}

bool Client::send(const void *data,unsigned size){
	unsigned sent=0;
	while(sent!=size){
		sent+=tcp.send_nonblock((char*)data+sent,size-sent);

		if(!parent.running()||tcp.error())
			return false;

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return parent.running()&&!tcp.error();
}

bool Client::recv(void *data,unsigned size){
	int got=0;
	while(got!=size){
		got+=tcp.recv_nonblock((char*)data+got,size-got);

		if(!parent.running()||tcp.error())
			return false;

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return parent.running()&&!tcp.error();
}

void Client::disconnect(){
	disconnected.store(true);
}
