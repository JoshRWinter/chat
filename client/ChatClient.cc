#include "ChatClient.h"

ChatClient::ChatClient(const std::string &dbpath):service(dbpath){
}

ChatClient::~ChatClient(){
}

bool ChatClient::connected()const{
	return service.is_connected();
}

// connect to server
void ChatClient::connect(const std::string &target,const std::string &myname,std::function<void(bool,const std::string&)> callback){
	auto unit=new ChatWorkUnitConnect(target,myname,callback);
	service.add_work(unit);
}

// refresh the chat list
void ChatClient::list_chats(std::function<void(std::vector<Chat>)> fn){
	auto unit=new ChatWorkUnitListChats(fn);
	service.add_work(unit);
}

// add a new chat to the server
void ChatClient::newchat(const std::string &name,const std::string &desc,std::function<void(bool)> callback){
	auto unit=new ChatWorkUnitNewChat(name,desc,callback);
	service.add_work(unit);
}

// subscribe to a chat
void ChatClient::subscribe(const std::string &name,std::function<void(bool,std::vector<Message>)> success_callback,std::function<void(Message)> msg_callback){
	auto unit=new ChatWorkUnitSubscribe(name,success_callback,msg_callback);
	service.add_work(unit);
}

// send a text message
void ChatClient::send(const std::string &text, std::function<void(bool,const std::string&)> fn){
	auto unit=new ChatWorkUnitMessage(MessageType::TEXT,text,NULL,0,fn);
	service.add_work(unit);
}

// send an image
void ChatClient::send_image(const std::string &filename, unsigned char *buffer, int size, std::function<void(bool,const std::string&)> fn){
	auto unit=new ChatWorkUnitMessage(MessageType::IMAGE, filename, buffer, size, fn);
	service.add_work(unit);
}

// send a file
void ChatClient::send_file(const std::string &filename, unsigned char *buffer, int size, std::function<void(bool,const std::string&)> fn){
	auto unit=new ChatWorkUnitMessage(MessageType::FILE, filename, buffer, size, fn);
	service.add_work(unit);
}

// request a file from the server
void ChatClient::get_file(unsigned long long msgid, std::function<void(const unsigned char*,int)> fn){
	auto unit=new ChatWorkUnitGetFile(msgid, fn);
	service.add_work(unit);
}
