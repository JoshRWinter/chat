#include "ChatClient.h"

ChatClient::~ChatClient(){
}

// connect to server
void ChatClient::connect(const std::string &target,const std::string &myname,std::function<void(bool,std::vector<Chat>)> callback){
	ChatWorkUnit *unit=new ChatWorkUnit;
	ChatWorkUnit::connect connect;

	unit->type=WorkUnitType::CONNECT;
	connect.target=target;
	connect.myname=myname;
	connect.callback=std::move(callback);

	unit->work=connect;
	service.add_work(unit);
}

void ChatClient::newchat(const std::string &name,std::function<void(bool)> callback){
	ChatWorkUnit *unit=new ChatWorkUnit;
	ChatWorkUnit::newchat newchat;

	unit->type=WorkUnitType::NEW_CHAT;
	newchat.chatname=name;
	newchat.callback=std::move(callback);

	unit->work=newchat;
	service.add_work(unit);
}

// subscribe to a chat
void ChatClient::subscribe(const Chat &desired,std::function<void(bool)> success_callback,std::function<void(Message)> msg_callback){
	ChatWorkUnit *unit=new ChatWorkUnit;
	ChatWorkUnit::subscribe subscribe;

	unit->type=WorkUnitType::SUBSCRIBE;
	subscribe.id=desired.id;
	subscribe.callback=std::move(success_callback);
	subscribe.msgcallback=std::move(msg_callback);

	unit->work=subscribe;
	service.add_work(unit);
}

void ChatClient::send(Message &msg){
	ChatWorkUnit *unit=new ChatWorkUnit;

	unit->type=WorkUnitType::MESSAGE;

	unit->work=msg;
	service.add_work(unit);
}
