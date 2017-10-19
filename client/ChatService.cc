#include <chrono>

#include <time.h>

#include "ChatService.h"
#include "log.h"

ChatService::ChatService():
	working(true),
	work_unit_count(0),
	handle(std::ref(*this))
{}

ChatService::~ChatService(){
	working.store(false);
	handle.join();

	// clear the work list
	while(units.size()>0){
		const ChatWorkUnit *unit=units.front();
		units.pop();
		delete unit;
	}
}

// accessed from multiple threads
// takes ownership of <unit>
void ChatService::add_work(const ChatWorkUnit *unit){
	std::lock_guard<std::mutex> lock(mutex);

	units.push(unit);
	++work_unit_count;
}

// entry point for the worker thread
void ChatService::operator()(){
	try{
		loop();
	}catch(const NetworkException &e){
		reconnect();
	}catch(const ShutdownException &e){
		if(work_unit_count.load()>0)
			log_error(std::string("shutting down with ")+std::to_string(work_unit_count.load())+" commands left in the queue!");
	}
}

void ChatService::send(const void *data,int size){
	int sent=0;
	while(sent!=size){
		sent+=tcp.send_nonblock((char*)data+sent,size-sent);

		if(!tcp)
			throw NetworkException();

		if(!working.load())
			throw ShutdownException();
	}
}

void ChatService::recv(void *data,int size){
	int got=0;
	while(got!=size){
		got+=tcp.recv_nonblock((char*)data+got,size-got);

		if(!tcp)
			throw NetworkException();

		if(!working.load())
			throw ShutdownException();
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

// work unit loop
void ChatService::loop(){
	while(working.load()){
		// process work units
		if(work_unit_count.load()>0){
			const ChatWorkUnit *unit=get_work();

			try{
				switch(unit->type){
				case WorkUnitType::CONNECT:
					process_connect(*dynamic_cast<const ChatWorkUnitConnect*>(unit));
					break;
				case WorkUnitType::NEW_CHAT:
					process_newchat(*dynamic_cast<const ChatWorkUnitNewChat*>(unit));
					break;
				case WorkUnitType::SUBSCRIBE:
					process_subscribe(*dynamic_cast<const ChatWorkUnitSubscribe*>(unit));
					break;
				case WorkUnitType::MESSAGE:
					process_send_text(*dynamic_cast<const ChatWorkUnitMessageText*>(unit));
					break;
				}
			}catch(const std::exception &e){
				log_error(e.what());
				// add the unit back to the queue to retry again later
				add_work(unit);
				throw;
			}

			// unit was processed successfully
			delete unit;
		}

		// see if the server has anything to say
		recv_server_cmd();

		std::this_thread::sleep_for(std::chrono::milliseconds(60));
	}
}

void ChatService::recv_server_cmd(){
	if(tcp.peek()<sizeof(ServerCommand))
		return;

	ServerCommand type;
	recv(&type,sizeof(type));

	switch(type){
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
	case ServerCommand::HEARTBEAT:
		// ignore
		break;
	default:
		// illegal
		log_error(std::string("received an illegal command from the server: ")+std::to_string(static_cast<uint8_t>(type)));
	}
}

// try to reconnect
void ChatService::reconnect(){
	log_error("lost connection! attempting to reconnect");

	bool reconnected=false;
	while(!reconnected){
		reconnected=tcp.connect();

		// see if services needs to shut down
		if(!working.load())
			throw ShutdownException();

		// don't burn out the cpu
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	log("reconnected successfully");
}

// get the first command in the list, then erase it from the list, the return it
const ChatWorkUnit *ChatService::get_work(){
	std::lock_guard<std::mutex> lock(mutex);

	const ChatWorkUnit *unit=units.front();
	units.pop();
	--work_unit_count;

	return unit;
}

// connect the client to server
void ChatService::process_connect(const ChatWorkUnitConnect &unit){
	callback.connect=unit.callback;

	// connect to server
	if(tcp.target(unit.target,CHAT_PORT)){
		time_t current=time(NULL);
		bool connected=false;
		// 5 second timeout
		while(time(NULL)-current<5&&!connected){
			connected=tcp.connect();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}

		if(connected){
			// introduce myself
			name=unit.myname;
			clientcmd_introduce();

			// request chat list
			clientcmd_list_chats();
		}
	}
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
	clientcmd_subscribe(unit.id,unit.name,0);
}

// send a message
void ChatService::process_send_text(const ChatWorkUnitMessageText &unit){
	Message msg(0,MessageType::TEXT,unit.text,name,NULL,0);
	clientcmd_message(msg);
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
void ChatService::clientcmd_subscribe(unsigned long long id,const std::string &chatname,unsigned long long latest){
	ClientCommand type=ClientCommand::SUBSCRIBE;
	send(&type,sizeof(type));

	send(&id,sizeof(id));
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
		send(msg.raw,msg.raw_size);
	}
}

// recv a list of chats from server
// implements ServerCommand::LIST_CHATS
void ChatService::servercmd_list_chats(){
	// get the server's name
	servername=get_string();
	log(std::string("servername is ")+servername);

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

	callback.connect(true,list);
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

		msgs.push_back({id,type,msg,sender,raw,raw_size});
	}

	callback.subscribe(true,msgs);
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

	Message message(id,type,msg,sender,raw,raw_size);

	callback.message(message);
}
