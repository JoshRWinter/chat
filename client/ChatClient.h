#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>

#include "ChatService.h"

class ChatClient{
public:
	ChatClient()=default;
	~ChatClient();
	bool connect(const std::string&,unsigned short);

private:
	ChatService service;
};

#endif // CHATCLIENT_H
