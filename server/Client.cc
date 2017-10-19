#include <functional>
#include <cstdint>

#include "Client.h"
#include "../chat.h"
#include "log.h"

Client::Client(Server &p,int sockfd):
	parent(p),
	tcp(sockfd),
	disconnected(false),
	out_queue_len(0),
	last_heartbeat(0),
	thread(std::ref(*this)) // start a separate event thread for this client (operator())
{}

// entry point for the client thread
void Client::operator()(){
	try{
		loop();
	}catch(const NetworkException &e){
		// ignore
	}catch(const ShutdownException &e){
		log(std::string("kicking ") + name);
	}catch(const ClientKickException &e){
		log_error(e.what());
	}

	disconnected.store(true);
}

// join the client thread (this fn is called from server thread)
void Client::join(){
	thread.join();
}

const std::string &Client::get_name()const{
	return name;
}

// has this client been disconnected (called from server thread)
bool Client::dead()const{
	return disconnected.load();
}

// is a client subscribed to <chat>
bool Client::is_subscribed(const Chat &chat){
	return chat==subscribed;
}

// kick this client
void Client::kick(const std::string &reason)const{
	throw ClientKickException(std::string("kicking ")+name+" because \""+reason+"\"");
}

// add a message to the out queue
void Client::addmsg(const Message &msg){
	std::lock_guard<std::mutex> lock(out_queue_lock);

	out_queue.push(msg);
	++out_queue_len;
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
	unsigned got=0;
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

		// recv client commands
		recv_command();

		// empty the out queue
		dispatch();

		Server::sleep();
	}
}

// empty the out queue
void Client::dispatch(){
	if(out_queue_len.load()<1)
		return;

	std::lock_guard<std::mutex> lock(out_queue_lock);
	while(out_queue.size()>0){
		const Message &msg=out_queue.front();

		// dispatch
		servercmd_message(msg);

		out_queue.pop();
	}

	out_queue_len.store(0);
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
		last_heartbeat=current;
	}
}

// subscribe the client to chat with name <name> and id <cid>
bool Client::subscribe(unsigned long long cid,const std::string &name){
	std::vector<Chat> chats=parent.get_chats();

	// find the proper chat
	for(const Chat &chat:chats){
		if(chat.id==cid&&chat.name==name){
			subscribed=chat;
			return true;
		}
	}

	return false;
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

// recv clients name
// implements ClientCommand::INTRODUCE
void Client::clientcmd_introduce(){
	name=get_string();
}

// lists the chats
// implements ClientCommand::LIST_CHATS
void Client::clientcmd_list_chats(){
	std::vector<Chat> chats=parent.get_chats();

	// execute ServerCommand::LIST_CHATS
	servercmd_list_chats(chats);
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
	bool valid=parent.valid_table_name(name);

	// tell the client
	servercmd_new_chat(valid);

	// create the new chat
	if(valid)
		parent.new_chat({0,name,creator,description});
}

// allow the client to subscribe to a chat
// implements ClientCommand::SUBSCRIBE
void Client::clientcmd_subscribe(){
	// get the id of the desired chat
	std::uint64_t cid;
	recv(&cid,sizeof(cid));

	// get the name of the desired chat
	std::string name=get_string();

	// try to subscribe the client
	bool success=subscribe(cid,name);

	// recv the max message id in that chat
	std::uint64_t max;
	recv(&max,sizeof(max));

	// execute ServerCommand::SUBSCRIBE
	servercmd_subscribe(success,max);
}

// client is sending a message
// implements ClientCommand::MESSAGE
void Client::clientcmd_message(){
	// receive the msg type
	MessageType type;
	recv(&type,sizeof(type));

	switch(type){
	case MessageType::FILE:
	case MessageType::IMAGE:
	case MessageType::TEXT:
		break;
	default:
		// illegal
		kick("illegal message type received: "+std::to_string(static_cast<uint8_t>(type)));
		break;
	}

	std::string message=get_string();

	// raw size
	decltype(Message::raw_size) raw_size;
	recv(&raw_size,sizeof(raw_size));

	unsigned char *raw=NULL;
	if(raw_size>0){
		raw=new unsigned char[raw_size];
		recv(raw,raw_size);
	}

	// don't let messages of zero length through
	if(message.length()==0)
		return;

	Message msg(0,type,message,name,raw,raw_size);

	parent.new_msg(subscribed,msg);
}

// send the client a list of chats
// implements ServerCommand::LIST_CHATS
void Client::servercmd_list_chats(const std::vector<Chat> &chats){
	ServerCommand type=ServerCommand::LIST_CHATS;
	send(&type,sizeof(type));

	send_string(parent.get_name());

	// send how many chats
	std::uint64_t count=chats.size();
	send(&count,sizeof(count));

	// send each chat
	for(const Chat &chat:chats){
		send(&chat.id,sizeof(chat.id));
		send_string(chat.name);
		send_string(chat.creator);
		send_string(chat.description);
	}
}

// tell the client if their new chat request worked or not
// implements ServerCommand::NEW_CHAT
void Client::servercmd_new_chat(bool success){
	ServerCommand type=ServerCommand::NEW_CHAT;
	send(&type,sizeof(type));

	uint8_t worked=success?1:0;
	send(&worked,sizeof(worked));
}

// send the client receipt of successful subscription, and messages since their last connect
// implements ServerCommand::SUBSCRIBE
void Client::servercmd_subscribe(bool success,unsigned long long max){
	ServerCommand type=ServerCommand::SUBSCRIBE;
	send(&type,sizeof(type));

	// first of all, tell the client if their subscription worked or not
	std::uint8_t worked=success?1:0;
	send(&worked,sizeof(worked));

	if(!success)
		return;

	// send all messages in the chat where message.id > max
	// basically getting the client back up to date since they were last connected
	std::vector<Message> since=parent.get_messages_since(max,subscribed.name);

	// send the count
	std::uint64_t count=since.size();
	send(&count,sizeof(count));

	// send <count> messages
	for(const Message &msg:since){
		// send id
		send(&msg.id,sizeof(msg.id));

		// send the message type
		send(&msg.type,sizeof(type));

		// send msg
		send_string(msg.msg);

		// send sender
		send_string(msg.sender);

		// send raw
		std::uint64_t raw_size=0;
		send(&raw_size,sizeof(raw_size));

		if(raw_size>0){
			send(msg.raw,raw_size);
		}
	}
}

// send the client a message
// implements ServerCommand::MESSAGE
void Client::servercmd_message(const Message &msg){
	ServerCommand type=ServerCommand::MESSAGE;
	send(&type,sizeof(type));

	// id
	send(&msg.id,sizeof(msg.id));

	// type
	send(&msg.type,sizeof(msg.type));

	send_string(msg.msg);
	send_string(msg.sender);

	send(&msg.raw_size,sizeof(msg.raw_size));
	send(msg.raw,msg.raw_size);
}

// send the client a heartbeat to see if they are disconnected
// implements ServerCommand::HEARTBEAT
void Client::servercmd_heartbeat(){
	ServerCommand type=ServerCommand::HEARTBEAT;
	send(&type,sizeof(type));
}
