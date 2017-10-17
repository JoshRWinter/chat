#include <functional>
#include <cstdint>

#include "Client.h"
#include "../chat.h"
#include "log.h"

Client::Client(Server &p,int sockfd):
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
		loop();
	}catch(const NetworkException &e){
		log(name + " has disconnected");
	}catch(const ShutdownException &e){
		log(std::string("kicking ") + name);
	}catch(const ClientKickException &e){
		log_error(e.what());
	}

	disconnect();
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

// kick this client
void Client::kick(const std::string &reason)const{
	throw ClientKickException(std::string("kicking ")+name+" because \""+reason+"\"");
}

bool Client::subscribe(int cid,const std::string &name){
	std::vector<Chat> chats=parent.get_chats();

	for(const Chat &chat:chats){
		if(chat.id==cid&&chat.name==name){
			subscribed=chat;
			return true;
		}
	}

	return false;
}

// main processing loop for client
void Client::loop(){
	for(;;){
		if(!parent.running())
			throw ShutdownException();

		heartbeat();

		recv_command();

		Server::sleep();
	}
}

// recv commands from the client
void Client::recv_command(){
	if(tcp.peek()<sizeof(ClientCommand))
		return;

	ClientCommand type;
	recv(&type,sizeof(type));

	switch(type){
	case ClientCommand::LIST_CHATS:
		clientcmd_list_chats();
		break;
	case ClientCommand::MESSAGE:
		clientcmd_message();
		break;
	case ClientCommand::SUBSCRIBE:
		clientcmd_subscribe();
		break;
	case ClientCommand::INTRODUCE:
		clientcmd_introduce();
		break;
	case ClientCommand::NEW_CHAT:
		clientcmd_newchat();
		break;
	default:
		// illegal
		kick(std::string("illegal command received from client: ")+std::to_string(static_cast<std::uint8_t>(type)));
		break;
	}
}

// send a heartbeat to the client
void Client::heartbeat(){
	time_t current=time(NULL);

	if(current<last_heartbeat) // user messed with system clock
		last_heartbeat=0;

	if(current-last_heartbeat>HEARTBEAT_FREQUENCY){
		servercmd_heartbeat();
		//last_heartbeat=current;
	}
}

void Client::disconnect(){
	disconnected.store(true);
}

// get a string off the network
std::string Client::get_string(){
	// get the size
	std::uint32_t size;
	recv(&size,sizeof(size));

	// get <size> characters
	std::vector<char> raw(size+1);
	recv(&raw[0],size);
	raw[size]=0;

	return {&raw[0]};
}

// send a string on the network
void Client::send_string(const std::string &str){
	// send the size
	std::uint32_t size=str.length();
	send(&size,sizeof(size));

	// send the string
	send(str.c_str(),size);
}

// lists the chats
// implements ClientCommand::LIST_CHATS
void Client::clientcmd_list_chats(){
	std::vector<Chat> chats=parent.get_chats();

	std::uint64_t count=chats.size();
	send(&count,sizeof(count));

	for(const Chat &chat:chats){
		send(&chat.id,sizeof(chat.id));
		send_string(chat.name);
		send_string(chat.creator);
		send_string(chat.description);
	}
}

// client is sending a message
// implements ClientCommand::MESSAGE
void Client::clientcmd_message(){
	// receive the msg type
	std::uint8_t type;
	recv(&type,sizeof(type));

	MessageType mtype=static_cast<MessageType>(type);

	switch(mtype){
	case MessageType::TEXT:
	case MessageType::FILE:
	case MessageType::IMAGE:
		break;
	default:
		// illegal
		kick("illegal message type received: "+std::to_string(type));
		break;
	}

	// receive the message
	std::string message=get_string();

	log(name+" says "+message);
}

// allow the client to subscribe to a chat
// implements ClientCommand::SUBSCRIBE
void Client::clientcmd_subscribe(){
	// get the id of the desired chat
	std::uint64_t cid;
	recv(&cid,sizeof(cid));

	// get the name of the desired chat
	std::string name=get_string();

	std::uint8_t success=subscribe(cid,name)?1:0;
	send(&success,sizeof(success));

	// recv the max message id in that chat
	std::uint64_t max;
	recv(&max,sizeof(max));

	// send all messages in the chat where message.id > max
	// basically getting the client back up to date since they were last connected
	std::vector<Message> since=parent.get_messages_since(max,subscribed.name);

	// send the count
	std::uint64_t count=since.size();

	// send <count> messages
	for(const Message &msg:since){
		// send the message type
		std::uint8_t type=static_cast<std::uint8_t>(msg.type);
		send(&type,sizeof(type));

		// send msg
		send_string(msg.msg);

		// send sender
		send_string(msg.sender);

		// send raw
		std::uint64_t raw_size=0;
		send(&raw_size,sizeof(raw_size));
	}
}

// server allows client to create new chats
// implements ClientCommand::NEW_CHAT
void Client::clientcmd_newchat(){
	// recv the name of the chat
	std::string name=get_string();

	// recv the creator of the chat
	const std::string creator=get_string();

	// recv the description
	const std::string description=get_string();

	// validate the table name
	std::uint8_t valid=parent.valid_table_name(name)?1:0;
	// tell the client
	send(&valid,sizeof(valid));
	if(valid)
		parent.new_chat({0,name,creator,description});
}

// recv clients name
// implements ClientCommand::INTRODUCE
void Client::clientcmd_introduce(){
	name=get_string();
	log(name+" has connected");
}

// send the client a heartbeat to see if they are disconnected
// implements ServerCommand::HEARTBEAT
void Client::servercmd_heartbeat(){
	return;
	ServerCommand type=ServerCommand::HEARTBEAT;

	send(&type,sizeof(type));
}
