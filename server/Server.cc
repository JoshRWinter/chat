#include <string>

#include "log.h"
#include "Server.h"

Server::Server(unsigned short port,const std::string &dbname):tcp(port),db(dbname){
	good.store(true);
	if(!tcp)
		throw ServerException(std::string("can't bind to port ")+std::to_string(port));

	servername=db.get_name();
	chats=db.get_chats();
}

Server::~Server(){
	good.store(false);

	// join all the client threads
	for(std::unique_ptr<Client> &client:client_list)
		client->join();
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
			client->join();
			it=client_list.erase(it);
			continue;
		}

		++it;
	}
}

bool Server::running()const{
	return good.load();
}

const std::string &Server::get_name(){
	return servername;
}

// return a copy of the chats vector
std::vector<Chat> Server::get_chats(){
	std::lock_guard<std::mutex> lock(mutex);
	return chats;
}

// create a new chat
void Server::new_chat(const Chat &chat){
	std::lock_guard<std::mutex> lock(mutex);

	try{
		db.new_chat(chat);
		log(chat.creator + " has created a new chat: \"" + chat.name + "\" description: \"" + chat.description + "\"");
	}catch(const DatabaseException &e){
		log_error(e.what());
	}

	chats=db.get_chats();
}

void Server::new_msg(const Chat &chat,Message &msg){
	std::lock_guard<std::mutex> lock(mutex);

	// insert into the database
	try{
		msg.id=db.new_msg(chat,msg);
	}catch(const DatabaseException &e){
		log_error(e.what());
		return;
	}

	// inform all subscribed clients of the new message
	for(std::unique_ptr<Client> &client:client_list){
		if(client->is_subscribed(chat))
			client->addmsg(msg);
	}
}

std::vector<Message> Server::get_messages_since(unsigned long long id,const std::string &name){
	std::lock_guard<std::mutex> lock(mutex);

	return db.get_messages_since(id,name);
}

bool Server::valid_table_name(const std::string &name){
	return db.valid_table_name(name);
}

void Server::sleep(){
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// accept a new client
void Server::new_client(int connector){
	client_list.push_back({std::make_unique<Client>(*this,connector)});
}
