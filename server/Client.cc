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
	last_sent_heartbeat(0),
	last_received_heartbeat(time(NULL)),
	name("anonymous"),
	thread(std::ref(*this)) // start a separate event thread for this client (operator())
{}

// entry point for the client thread
void Client::operator()(){
	try{
		loop();
	}catch(const NetworkException &e){
		// ignore
	}catch(const ShutdownException &e){
		log("kicking " + name);
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

		// check if the remote client has timed out
		check_timeout();
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
	if(!tcp.poll_recv(350))
		return;

	ClientCommand type;
	recv(&type,sizeof(type));

	switch(type){
	case ClientCommand::INTRODUCE:
		clientcmd_introduce();
		break;
	case ClientCommand::LIST_CHATS:
		clientcmd_list_chats();
		break;
	case ClientCommand::MESSAGE:
		clientcmd_message();
		break;
	case ClientCommand::SUBSCRIBE:
		clientcmd_subscribe();
		break;
	case ClientCommand::NEW_CHAT:
		clientcmd_newchat();
		break;
	case ClientCommand::GET_FILE:
		clientcmd_get_file();
		break;
	case ClientCommand::HEARTBEAT:
		last_received_heartbeat = time(NULL);
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

	if(current<last_sent_heartbeat) // user messed with system clock
		last_sent_heartbeat=0;

	if(current-last_sent_heartbeat>HEARTBEAT_FREQUENCY){
		servercmd_heartbeat();
		last_sent_heartbeat=current;
	}
}

void Client::check_timeout(){
	const time_t current = time(NULL);

	if(current < last_received_heartbeat) // user messed with system clock
		last_received_heartbeat = 0;

	if(current - last_received_heartbeat > TIMEOUT_SECONDS)
		throw NetworkException();
}

// subscribe the client to chat with name <name> and id <cid>
bool Client::subscribe(const std::string &name){
	std::vector<Chat> chats=parent.get_chats();

	// find the proper chat
	for(const Chat &chat:chats){
		if(chat.name==name){
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

	// validate name
	name=parent.validate_name(*this);

	servercmd_introduce();
}

// format the bytes as KB or MB
std::string Client::format(int bytes){
	const char BUFFER_SIZE=30;
	char buffer[BUFFER_SIZE];

	if(bytes>1024*1024){
		double megabytes=bytes/1024.0/1024.0;
		std::snprintf(buffer, BUFFER_SIZE, "%.1fMB", megabytes);
	}
	else{
		double kilobytes=bytes/1024.0;
		std::snprintf(buffer, BUFFER_SIZE, "%.1fKB", kilobytes);
	}

	return buffer;
}

std::string Client::strip_new_lines(const std::string &str){
	std::string line = str;

	for(char &c : line)
		if(c == '\n' || c == '\r')
			c = ' ';

	return line;
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
	const std::string name=Client::strip_new_lines(get_string());

	// recv the creator of the chat
	const std::string creator=Client::strip_new_lines(get_string());

	// recv the description
	const std::string description=Client::strip_new_lines(get_string());

	// create the new chat
	servercmd_new_chat(parent.new_chat({0,name,creator,description}));
}

// allow the client to subscribe to a chat
// implements ClientCommand::SUBSCRIBE
void Client::clientcmd_subscribe(){
	// get the name of the desired chat
	std::string name=get_string();

	// try to subscribe the client
	bool success=subscribe(name);

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

	// make sure raw size isn't too big
	if(type==MessageType::IMAGE){
		if(raw_size>MAX_IMAGE_BYTES){
			delete[] raw;
			servercmd_message_receipt(false, "Images larger than "+Client::format(MAX_IMAGE_BYTES)+" are not allowed.");
			return;
		}

		message+=" ("+Client::format(raw_size)+")";
	}
	else if(type==MessageType::FILE){
		if(raw_size>MAX_FILE_BYTES){
			delete[] raw;
			servercmd_message_receipt(false, "Files larger than "+Client::format(MAX_FILE_BYTES)+" are not allowed.");
			return;
		}

		message+=" ("+Client::format(raw_size)+")";
	}
	else{
		if(raw_size>0){
			delete[] raw;
			servercmd_message_receipt(false, "The \"raw\" field is not allowed for general text messages.\nThis likely indicates a client implementation error.");
			return;
		}
	}

	// don't let messages of zero length through
	if(message.length()==0){
		servercmd_message_receipt(false, "No zero-length messages!");
		delete[] raw;
		return;
	}

	Message msg(0,type,time(NULL),message,name,raw,raw_size);

	if(subscribed){
		parent.new_msg(subscribed.value(),msg);
		servercmd_message_receipt(true,{});
	}
	else
		servercmd_message_receipt(false, "You are not subscribed to any chat sessions!");
}

// client is requesting a file
// implements ClientCommand::SEND_FILE
void Client::clientcmd_get_file(){
	std::uint64_t id;
	recv(&id, sizeof(id));

	std::vector<unsigned char> buffer=parent.get_file(id, subscribed.value().id);
	servercmd_send_file(buffer);
}

// send the client their (validated) name back
// implements ServerCommand::INTRODUCE
void Client::servercmd_introduce(){
	ServerCommand type=ServerCommand::INTRODUCE;
	send(&type, sizeof(type));

	send_string(name);
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
	std::vector<Message> since=parent.get_messages_since(max,subscribed.value().id);

	// send the count
	std::uint64_t count=since.size();
	send(&count,sizeof(count));

	// send <count> messages
	for(const Message &msg:since){
		// send id
		send(&msg.id,sizeof(msg.id));

		// send the message type
		send(&msg.type,sizeof(type));

		// send the unixtime
		send(&msg.unixtime,sizeof(msg.unixtime));

		// send msg
		send_string(msg.msg);

		// send sender
		send_string(msg.sender);

		// send raw
		std::uint64_t raw_size=msg.raw_size;
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

	// unixtime
	send(&msg.unixtime, sizeof(msg.unixtime));

	send_string(msg.msg);
	send_string(msg.sender);

	send(&msg.raw_size,sizeof(msg.raw_size));
	send(msg.raw,msg.raw_size);
}

// tell the client whether their sent message was successful
// implements ServerCommand::MESSAGE_RECEIPT
void Client::servercmd_message_receipt(bool success, const std::string &msg){
	ServerCommand type=ServerCommand::MESSAGE_RECEIPT;
	send(&type, sizeof(type));

	std::uint8_t worked=success?1:0;
	send(&worked, sizeof(worked));

	if(!success){
		// send the error message description
		send_string(msg);
	}
}

// send a file to the client
void Client::servercmd_send_file(const std::vector<unsigned char> &buffer){
	ServerCommand type=ServerCommand::SEND_FILE;
	send(&type, sizeof(type));

	std::uint64_t size=(std::uint64_t)buffer.size();
	send(&size, sizeof(size));

	send(&buffer[0], size);
}

// send the client a heartbeat to see if they are disconnected
// implements ServerCommand::HEARTBEAT
void Client::servercmd_heartbeat(){
	ServerCommand type=ServerCommand::HEARTBEAT;
	send(&type,sizeof(type));
}
