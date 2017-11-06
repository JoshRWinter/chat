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
	void connect(const std::string&,const std::string&,std::function<void(bool)>);
	void list_chats(std::function<void(std::vector<Chat>)>);
	void newchat(const std::string&,const std::string&,std::function<void(bool)>);
	void subscribe(const std::string&,std::function<void(bool,std::vector<Message>)>,std::function<void(Message)>);
	void send(const std::string&, std::function<void(bool,const std::string&)> fn);
	void send_image(const std::string&, unsigned char*, int, std::function<void(bool,const std::string&)> fn);
	void send_file(const std::string&, unsigned char*, int, std::function<void(bool,const std::string&)> fn);
	void get_file(unsigned long long, std::function<void(const unsigned char*,int)>);
	std::string name()const;

private:
	ChatService service;
	std::string clientname;
};

#endif // CHATCLIENT_H
