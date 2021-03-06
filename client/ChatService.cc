#include <chrono>
#include <climits>

#include <time.h>

#include "ChatService.h"
#include "log.h"

ChatService::ChatService(const std::string &dbpath):
	db(dbpath),
	working(true),
	connected(false),
	last_heartbeat(0),
	handle(std::ref(*this))
{
	callback.percent = NULL;
}

ChatService::~ChatService(){
	working.store(false);
	handle.join();
}

// accessed from multiple threads
// takes ownership of <unit>
void ChatService::add_work(const ChatWorkUnit *unit){
	work_queue.push(unit);
}

// entry point for the worker thread
void ChatService::operator()(){
	try{
		loop();
	}catch(const NetworkException &e){
		connected.store(false);
		reconnect();
		connected.store(true);
		// recurse
		this->operator()();
	}catch(const ShutdownException &e){
		const int leftover = work_queue.count();

		if(leftover>0)
			log_error(std::string("shutting down with ")+std::to_string(leftover)+" commands left in the queue!");
	}
	catch(const lite3::exception &e){
		log_error(e.what());
		// recurse
		this->operator()();
	}

	connected.store(false);
}

void ChatService::send(const void *data,int size,std::atomic<int> *percent){
	int sent=0;
	while(sent!=size){
		const int sendblock = 4096;
		const int sendcap = size - sent > sendblock ? sendblock : (size - sent);
		sent+=tcp.send_nonblock((char*)data+sent,sendcap);

		if(!tcp){
			if(percent != NULL)
				percent->store(-1);
			throw NetworkException();
		}

		if(!working.load()){
			if(percent != NULL)
				percent->store(-1);
			throw ShutdownException();
		}

		// update percent
		if(percent != NULL)
			percent->store(((float)sent / size) * 100);
	}
}

void ChatService::recv(void *data,int size,std::atomic<int> *percent){
	int got=0;
	while(got!=size){
		got+=tcp.recv_nonblock((char*)data+got,size-got);

		if(!tcp){
			if(percent != NULL)
				percent->store(-1);
			throw NetworkException();
		}

		if(!working.load()){
			if(percent != NULL)
				percent->store(-1);
			throw ShutdownException();
		}

		// update percent
		if(percent != NULL)
			percent->store(((float)got / size) * 100);
	}
}

// send a string on the network
void ChatService::send_string(const std::string &str){
	// send the string length
	uint32_t size=str.length();
	send(&size,sizeof(size));

	// send the string
	send(str.c_str(),size);
}

// pull a string off the network
std::string ChatService::get_string(){
	// get the length of the string
	uint32_t size;
	recv(&size,sizeof(size));

	// get the string
	std::vector<char> raw(size+1);
	recv(&raw[0],size);
	raw[size]=0;

	return {&raw[0]};
}

bool ChatService::is_connected()const{
	return connected.load();
}

// work unit loop
void ChatService::loop(){
	while(working.load()){
		// process work units
		const ChatWorkUnit *unit = work_queue.pop_wait(200);
		if(unit!=NULL){
			switch(unit->type){
			case WorkUnitType::CONNECT:
				process_connect(*dynamic_cast<const ChatWorkUnitConnect*>(unit));
				break;
			case WorkUnitType::LIST_CHATS:
				process_list_chats(*dynamic_cast<const ChatWorkUnitListChats*>(unit));
				break;
			case WorkUnitType::NEW_CHAT:
				process_newchat(*dynamic_cast<const ChatWorkUnitNewChat*>(unit));
				break;
			case WorkUnitType::SUBSCRIBE:
				process_subscribe(*dynamic_cast<const ChatWorkUnitSubscribe*>(unit));
				break;
			case WorkUnitType::MESSAGE:
				process_send_message(*dynamic_cast<const ChatWorkUnitMessage*>(unit));
				break;
			case WorkUnitType::GET_FILE:
				process_get_file(*dynamic_cast<const ChatWorkUnitGetFile*>(unit));
				break;
			}

			// unit was processed successfully
			delete unit;
		}

		// if connected
		if(tcp){
			// see if the server has anything to say
			recv_server_cmd();

			// maybe send a heartbeat
			heartbeat();
		}
	}
}

void ChatService::recv_server_cmd(){
	if(tcp.peek()<sizeof(ServerCommand))
		return;

	ServerCommand type;
	recv(&type,sizeof(type));

	switch(type){
	case ServerCommand::INTRODUCE:
		servercmd_introduce();
		break;
	case ServerCommand::LIST_CHATS:
		servercmd_list_chats();
		break;
	case ServerCommand::NEW_CHAT:
		servercmd_new_chat();
		break;
	case ServerCommand::SUBSCRIBE:
		servercmd_subscribe();
		break;
	case ServerCommand::MESSAGE:
		servercmd_message();
		break;
	case ServerCommand::MESSAGE_RECEIPT:
		servercmd_message_receipt();
		break;
	case ServerCommand::SEND_FILE:
		servercmd_send_file();
		break;
	case ServerCommand::HEARTBEAT:
		// ignore
		break;
	default:
		// illegal
		log_error(std::string("received an illegal command from the server: ")+std::to_string(static_cast<uint8_t>(type)));
	}
}

// maybe send a heatbeat to see if the server is still there
void ChatService::heartbeat(){
	time_t current=time(NULL);

	// someone is messing with system clock
	if(last_heartbeat>current)
		last_heartbeat=0;

	if(current-last_heartbeat>HEARTBEAT_FREQUENCY){
		last_heartbeat=current;
		clientcmd_heartbeat();
	}
}

// try to reconnect
void ChatService::reconnect(){
	try{
		log_error("lost connection! attempting to reconnect");

		tcp.target(target,CHAT_PORT);

		bool reconnected=false;
		while(!reconnected){
			reconnected=tcp.connect();

			// see if services needs to shut down
			if(!working.load())
				throw ShutdownException();

			// don't burn out the cpu
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}

		// reconnect to the server
		auto unit=new ChatWorkUnitConnect(target,name,callback.connect);
		add_work(unit);
		// resubscribe, if necessary
		if(chatname!=""){
			auto unit2=new ChatWorkUnitSubscribe(chatname,[](bool,std::vector<Message>){},callback.message);
			add_work(unit2);
		}

		log("reconnected successfully");
	}catch(const ShutdownException &e){}
}

// connect the client to server
void ChatService::process_connect(const ChatWorkUnitConnect &unit){
	callback.connect=unit.callback;

	// store this for later
	target=unit.target;

	// connect to server
	if(tcp.target(unit.target,CHAT_PORT)){
		time_t current=time(NULL);
		bool success=false;
		// 5 second timeout
		while(time(NULL)-current<5&&!success){
			success=tcp.connect();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}

		if(success){
			// introduce myself
			connected.store(true);
			name=unit.myname;
			clientcmd_introduce();
		}
		else{
			unit.callback(false,{});
			tcp.close();
		}
	}
	else{
		unit.callback(false,{});
		tcp.close();
	}
}

// refresh the chat list for the user
void ChatService::process_list_chats(const ChatWorkUnitListChats &unit){
	callback.chatlist=unit.callback;
	clientcmd_list_chats();
}

// ask the server to create new chat
void ChatService::process_newchat(const ChatWorkUnitNewChat &unit){
	callback.newchat=unit.callback;
	clientcmd_new_chat(unit.name,unit.desc);
}

// subscribe to a chat
void ChatService::process_subscribe(const ChatWorkUnitSubscribe &unit){
	callback.subscribe=unit.callback;
	callback.message=unit.msg_callback;
	clientcmd_subscribe(unit.name,db.get_latest_msg(unit.name));
	chatname=unit.name; // store chatname for later
}

// send a message
void ChatService::process_send_message(const ChatWorkUnitMessage &unit){
	callback.receipt=unit.callback;
	callback.percent=unit.percent;

	Message msg(0,unit.type,0,unit.text,name,unit.raw,unit.raw_size);
	clientcmd_message(msg);
}

// request a file from the server
void ChatService::process_get_file(const ChatWorkUnitGetFile &unit){
	callback.file=unit.callback;
	callback.percent=unit.percent;
	clientcmd_get_file(unit.id);
}

// tell the server user's name
// implements ClientCommand::INTRODUCE
void ChatService::clientcmd_introduce(){
	ClientCommand type=ClientCommand::INTRODUCE;
	send(&type,sizeof(type));

	send_string(name);
}

// ask the server for list of chats
// implements ClientCommand::LIST_CHATS
void ChatService::clientcmd_list_chats(){
	ClientCommand type=ClientCommand::LIST_CHATS;
	send(&type,sizeof(type));
}

// tell the server to make a new chat
// implements ClientCommand::NEW_CHAT
void ChatService::clientcmd_new_chat(const std::string &chatname,const std::string &desc){
	ClientCommand type=ClientCommand::NEW_CHAT;
	send(&type,sizeof(type));

	send_string(chatname);
	send_string(name);
	send_string(desc);
}

// subscribe to a chat
// implements ClientCommand::SUBSCRIBE
void ChatService::clientcmd_subscribe(const std::string &chatname,unsigned long long latest){
	ClientCommand type=ClientCommand::SUBSCRIBE;
	send(&type,sizeof(type));

	send_string(chatname);
	// send the highest message that exists in the database
	std::uint64_t max=latest;
	send(&max,sizeof(max));
}

// send a message
// implements ClientCommand::MESSAGE
void ChatService::clientcmd_message(const Message &msg){
	ClientCommand type=ClientCommand::MESSAGE;
	send(&type,sizeof(type));

	send(&msg.type,sizeof(msg.type));
	send_string(msg.msg);

	// raw size
	send(&msg.raw_size,sizeof(msg.raw_size));
	if(msg.raw_size>0){
		send(msg.raw,msg.raw_size,callback.percent);
	}
}

// request a file from the server
// implements ClientCommand::GET_FILE
void ChatService::clientcmd_get_file(unsigned long long id){
	ClientCommand type=ClientCommand::GET_FILE;
	send(&type, sizeof(type));

	send(&id, sizeof(id));
}

// send a heartbeat
// implements ClientCommand::HEARTBEAT
void ChatService::clientcmd_heartbeat(){
	ClientCommand type=ClientCommand::HEARTBEAT;
	send(&type,sizeof(type));
}

// receive the validated name back from the server
// implements ServerCommand::INTRODUCE
void ChatService::servercmd_introduce(){
	name=get_string();

	callback.connect(true, name);
}

// recv a list of chats from server
// implements ServerCommand::LIST_CHATS
void ChatService::servercmd_list_chats(){
	// get the server's name
	servername=get_string();
	db.set_servername(servername);

	// recv the number of chats
	std::uint64_t count;
	recv(&count,sizeof(count));

	std::vector<Chat> list;

	for(unsigned i=0;i<count;++i){
		decltype(Chat::id) id;
		recv(&id,sizeof(id));

		const std::string &name=get_string();
		const std::string &creator=get_string();
		const std::string &description=get_string();

		list.push_back({id,name,creator,description});
	}

	callback.chatlist(list);
}

// recv receipt of previously created new chat
// implements ServerCommand::NEW_CHAT
void ChatService::servercmd_new_chat(){
	std::uint8_t worked;
	recv(&worked,sizeof(worked));

	callback.newchat(worked==1);
}

// recv receipt of subscription, and message since user last connected
// implements ServerCommand::SUBSCRIBE
void ChatService::servercmd_subscribe(){
	// see if the earlier subscribe command worked
	std::uint8_t worked;
	recv(&worked,sizeof(worked));
	if(!worked){
		callback.subscribe(false,{});
		return;
	}

	std::vector<Message> msgs;

	// get the number of messages
	std::uint64_t count;
	recv(&count,sizeof(count));

	for(unsigned long long i=0;i<count;++i){
		// id
		decltype(Message::id) id;
		recv(&id,sizeof(id));

		// type
		MessageType type;
		recv(&type,sizeof(type));

		// recv unix time
		std::int32_t unixtime;
		recv(&unixtime, sizeof(unixtime));

		const std::string &msg=get_string();
		const std::string &sender=get_string();

		// raw size
		decltype(Message::raw_size) raw_size;
		recv(&raw_size,sizeof(raw_size));

		unsigned char *raw=NULL;
		if(raw_size>0){
			// raw
			raw=new unsigned char[raw_size];
			recv(raw,raw_size);
		}

		msgs.push_back({id,type,unixtime,msg,sender,raw,raw_size});
	}

	// give the client messages that were already in this chat
	callback.subscribe(true,db.get_msgs(chatname));

	for(const Message &msg:msgs){
		db.newmsg(msg,chatname);
		callback.message(msg);
	}
}

// recv a message from the server
// implements ServerCommand::MESSAGE
void ChatService::servercmd_message(){
	// id
	decltype(Message::id) id;
	recv(&id,sizeof(id));

	// msg tpe
	MessageType type;
	recv(&type,sizeof(type));

	// unixtime
	std::int32_t unixtime;
	recv(&unixtime, sizeof(unixtime));

	const std::string &msg=get_string();
	const std::string &sender=get_string();

	// raw size
	decltype(Message::raw_size) raw_size;
	recv(&raw_size,sizeof(raw_size));

	unsigned char *raw=NULL;
	if(raw_size>0){
		raw=new unsigned char[raw_size];
		recv(raw,raw_size);
	}

	Message message(id,type,unixtime,msg,sender,raw,raw_size);

	// store it in the db
	db.newmsg(message,chatname);

	// tell the user
	callback.message(message);
}

// determine if server accepted previously sent message
// implements ServerCommand::MESSAGE_RECEIPT
void ChatService::servercmd_message_receipt(){
	std::uint8_t worked;
	recv(&worked, sizeof(worked));

	std::string err;
	if(worked==0){
		err=get_string();
	}

	callback.receipt(worked==1, err);
	callback.receipt=nullptr;
}

// server is sending file
// implements ServerCommand::SEND_FILE
void ChatService::servercmd_send_file(){
	std::uint64_t size;
	recv(&size, sizeof(size));

	std::unique_ptr<unsigned char[]> buffer(new unsigned char[size]);
	recv(buffer.get(), size, callback.percent);

	// handle errors
	if(size==0||size>INT_MAX){
		callback.file(NULL, 0);
		return;
	}

	// notify the user
	callback.file(buffer.get(), (int)size);
}
