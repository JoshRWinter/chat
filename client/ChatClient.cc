#include "ChatClient.h"

ChatClient::ChatClient(const std::string &dbpath):service(dbpath){
}

ChatClient::~ChatClient(){
}

// connect to server
void ChatClient::connect(const std::string &target,const std::string &myname,std::function<void(bool)> callback){
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
void ChatClient::send(const std::string &text){
	auto unit=new ChatWorkUnitMessage(MessageType::TEXT,text,NULL,0);
	service.add_work(unit);
}
