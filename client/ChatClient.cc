#include "ChatClient.h"

ChatClient::ChatClient(const std::string &dbpath){
}

ChatClient::~ChatClient(){
}

// connect to server
void ChatClient::connect(const std::string &target,const std::string &myname,std::function<void(bool,std::vector<Chat>)> callback){
	auto unit=new ChatWorkUnitConnect(target,myname,callback);
	service.add_work(unit);
}

void ChatClient::newchat(const std::string &name,const std::string &desc,std::function<void(bool)> callback){
	auto unit=new ChatWorkUnitNewChat(name,desc,callback);
	service.add_work(unit);
}

// subscribe to a chat
void ChatClient::subscribe(unsigned long long id,const std::string &name,std::function<void(bool,std::vector<Message>)> success_callback,std::function<void(Message)> msg_callback){
	auto unit=new ChatWorkUnitSubscribe(id,name,success_callback,msg_callback);
	service.add_work(unit);
}

// send a text message
void ChatClient::send(const std::string &text){
	auto unit=new ChatWorkUnitMessageText(text);
	service.add_work(unit);
}
