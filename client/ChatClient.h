#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>
#include <functional>
#include <vector>

#include "ChatService.h"

#ifdef _WIN32
class __declspec(dllexport) ChatClient{
#else
class ChatClient{
#endif // _WIN32
public:
	ChatClient(const std::string&);
	~ChatClient();
	bool connected()const;
	void connect(const std::string&,const std::string&,std::function<void(bool,const std::string&)>);
	void list_chats(std::function<void(std::vector<Chat>)>);
	void newchat(const std::string&,const std::string&,std::function<void(bool)>);
	void subscribe(const std::string&,std::function<void(bool,std::vector<Message>)>,std::function<void(Message)>);
	void send(const std::string&, std::function<void(bool,const std::string&)> fn);
	void send_image(const std::string&, unsigned char*, int, std::function<void(bool,const std::string&)> fn);
	void send_file(const std::string&, unsigned char*, int, std::function<void(bool,const std::string&)> fn);
	void get_file(unsigned long long, std::function<void(const unsigned char*,int)>);

private:
	ChatService service;
};

#endif // CHATCLIENT_H
