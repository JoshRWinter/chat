#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>
#include <functional>
#include <vector>

#include "ChatService.h"

class ChatClient{
public:
	ChatClient(const std::string&);
	~ChatClient();
	void connect(const std::string&,const std::string&,std::function<void(bool,std::vector<Chat>)>);
	void newchat(const std::string&,const std::string&,std::function<void(bool)>);
	void subscribe(unsigned long long,const std::string&,std::function<void(bool,std::vector<Message>)>,std::function<void(Message)>);
	void send(const std::string&);

private:
	ChatService service;
};

#endif // CHATCLIENT_H
