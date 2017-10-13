#include <string>

#include "log.h"
#include "Server.h"

Server::Server(unsigned short port,const std::string &dbname):tcp(port),db(dbname){
	good.store(true);
	if(!tcp)
		throw ServerException(std::string("can't bind to port ")+std::to_string(port));
}

Server::~Server(){
	good.store(false);

	// join all the client threads
	for(std::unique_ptr<Client> &client:client_list)
		client->get_thread().join();
}

void Server::accept(){
	int connector=tcp.accept();

	if(connector!=-1){
		// yay someone connected
		new_client(connector);
	}

	// remove dead clients from client_list
	for(auto it=client_list.begin();it!=client_list.end();){
		auto &client = *it;

		if(client->dead()){
			client->get_thread().join();
			it=client_list.erase(it);
			continue;
		}

		++it;
	}
}

std::vector<Chat> Server::get_chats(){
	std::lock_guard<std::mutex> lock(mutex);
	return db.get_chats();
}

void Server::new_chat(const Chat &chat){
	std::lock_guard<std::mutex> lock(mutex);
	try{
		db.new_chat(chat);
		log(chat.creator + " has created a new chat: \"" + chat.name + "\" description: \"" + chat.description + "\"");
	}catch(const DatabaseException &e){
		log_error(e.what());
	}
}

bool Server::running()const{
	return good.load();
}

void Server::new_client(int connector){
	client_list.push_back({std::make_unique<Client>(*this,connector)});
}
