#ifndef CHATWORKUNIT_H
#define CHATWORKUNIT_H

#include <functional>
#include <string>

#include "../chat.h"

enum class WorkUnitType:std::uint8_t{
	CONNECT, // connect to a server
	NEW_CHAT, // have the server make a new chat
	SUBSCRIBE, // subscribe to a chat
	MESSAGE // send a message
};

struct ChatWorkUnit{
	ChatWorkUnit(WorkUnitType t):type(t){}
	virtual ~ChatWorkUnit(){};
	const WorkUnitType type;
};

// for connecting to the server
struct ChatWorkUnitConnect:ChatWorkUnit{
	ChatWorkUnitConnect(const std::string &t,const std::string &n,std::function<void(bool,std::vector<Chat>)> fn)
	:ChatWorkUnit(WorkUnitType::CONNECT)
	,target(t)
	,myname(n)
	,callback(fn)
	{}

	const std::string target;
	const std::string myname;
	const std::function<void(bool,std::vector<Chat>)> callback;
};

// for creating a new chat
struct ChatWorkUnitNewChat:ChatWorkUnit{
	ChatWorkUnitNewChat(const std::string &n,const std::string &d,std::function<void(bool)> fn)
	:ChatWorkUnit(WorkUnitType::NEW_CHAT)
	,name(n)
	,desc(d)
	,callback(fn)
	{}

	const std::string name;
	const std::string desc;
	const std::function<void(bool)> callback;
};

// for subscribing to a chat
struct ChatWorkUnitSubscribe:ChatWorkUnit{
	ChatWorkUnitSubscribe(const std::string &n,std::function<void(bool,std::vector<Message>)> c,std::function<void(Message)> m)
	:ChatWorkUnit(WorkUnitType::SUBSCRIBE)
	,name(n)
	,callback(c)
	,msg_callback(m)
	{}

	const std::string name;
	const std::function<void(bool,std::vector<Message>)> callback;
	const std::function<void(Message)> msg_callback;
};

// for sending a message
struct ChatWorkUnitMessage:ChatWorkUnit{
	ChatWorkUnitMessage(MessageType t,const std::string &m,const unsigned char *r,unsigned long long rs)
	:ChatWorkUnit(WorkUnitType::MESSAGE)
	,type(t)
	,text(m)
	,raw_size(rs)
	{
		if(r!=NULL){
			raw=new unsigned char[raw_size];
			memcpy(raw,r,raw_size);
		}
		else
			raw=NULL;
	}
	~ChatWorkUnitMessage(){delete[] raw;}

	const MessageType type;
	const std::string text;
	unsigned char *raw;
	const unsigned long long raw_size;
};

#endif // CHATWORKUNIT_H
