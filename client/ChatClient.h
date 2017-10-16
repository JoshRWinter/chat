#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>
#include <functional>

#include "ChatService.h"

class ChatClient{
public:
	ChatClient()=default;
	~ChatClient();
	void connect(const std::string&,const std::string&,std::function<void(bool,std::vector<Chat>)>);
	void newchat(const std::string&,std::function<void(bool)>);
	void subscribe(const Chat&,std::function<void(bool)>,std::function<void(Message)>);
	void send(Message&);

private:
	ChatService service;
};

#endif // CHATCLIENT_H
