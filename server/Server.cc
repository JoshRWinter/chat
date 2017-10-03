#include <iostream>

#include "Server.h"

Server::Server(unsigned short port):tcp(port){
	good.store(true);
}

Server::~Server(){
	good.store(false);

	// join all the client threads
	for(std::unique_ptr<Client> &client:client_list)
		client->get_thread().join();
}

bool Server::operator!()const{
	return !tcp;
}

void Server::accept(){
	int connector=tcp.accept();

	if(connector!=-1){
		// yay someone connected
		new_client(connector);
		std::cout<<"someone connected"<<std::endl;
	}

	// remove dead clients from client_list
	for(std::vector<std::unique_ptr<Client>>::iterator it=client_list.begin();it!=client_list.end();){
		std::unique_ptr<Client> &client = *it;

		if(client->dead()){
			client->get_thread().join();
			std::cout<<client->get_name()<<" disconnected"<<std::endl;
			it=client_list.erase(it);
			continue;
		}

		++it;
	}
}

bool Server::running()const{
	return good.load();
}

void Server::new_client(int connector){
	client_list.push_back({std::make_unique<Client>(*this,connector)});
}
