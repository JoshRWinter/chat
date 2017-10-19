#ifndef CHAT_H
#define CHAT_H

#include <cstdint>
#include <memory>

#include <string.h>

#define CHAT_PORT 28859

#define MESSAGE_LEN_INT 510 // these two must
#define MESSAGE_LEN_STR "510" // be kept in sync

// command from the server
enum class ServerCommand:std::uint8_t{
	LIST_CHATS, // sending client list of chats
	NEW_CHAT, // sending client receipt of new chat
	SUBSCRIBE, // server is confirming successful subscription
	MESSAGE, // server is sending a client a message
	HEARTBEAT // server is sending a heartbeat to client
};

// command from the client
enum class ClientCommand:std::uint8_t{
	INTRODUCE, // client wants to introduce himself
	LIST_CHATS, // client wants the list of chats
	NEW_CHAT, // client wants to create a new chat
	SUBSCRIBE, // client wants to subscribe to a chat
	MESSAGE // client is sending a message
};

enum class MessageType:std::uint8_t{
	TEXT,
	IMAGE,
	FILE
};

// the message object
struct Message{
	Message():id(0),raw(NULL),raw_size(0){}
	Message(unsigned long long i,MessageType t,const std::string &m,const std::string &s,unsigned char *r,unsigned long long rs)
	:id(i),type(t),msg(m),sender(s),raw(r),raw_size(rs){}

	Message(const Message &rhs){ // copy constructor
		id=rhs.id;
		type=rhs.type;
		msg=rhs.msg;
		sender=rhs.sender;
		raw_size=rhs.raw_size;
		if(raw_size>0){
			raw=new unsigned char[raw_size];
			memcpy(raw,rhs.raw,raw_size);
		}
		else{
			raw=NULL;
		}
	}

	Message(Message &&other){ // move constructor
		id=other.id;
		type=other.type;
		msg=other.msg;
		sender=other.sender;
		raw_size=other.raw_size;
		raw=other.raw;

		other.raw=NULL;
	}

	~Message(){
		delete[] raw;
	}

	Message &operator=(Message &&rhs){ // move assignment
		delete[] raw;

		id=rhs.id;
		type=rhs.type;
		msg=std::move(rhs.msg);
		sender=std::move(rhs.sender);
		raw=rhs.raw;
		raw_size=rhs.raw_size;

		rhs.raw=NULL;
		rhs.raw_size=0;

		return *this;
	}

	Message &operator=(const Message &rhs){ // copy assignment
		delete[] raw;

		id=rhs.id;
		type=rhs.type;
		msg=rhs.msg;
		sender=rhs.sender;
		raw_size=rhs.raw_size;
		if(raw_size>0){
			raw=new unsigned char[rhs.raw_size];
			memcpy(raw,rhs.raw,rhs.raw_size);
		}
		else
			raw=NULL;

		return *this;
	}

	unsigned long long id;
	MessageType type;
	std::string msg;
	std::string sender;
	unsigned char *raw;
	unsigned long long raw_size;
};

// describes a "chat" the meta data for a group chat
struct Chat{
	Chat():id(0){}
	Chat(unsigned long long i,const std::string &n,const std::string &c,const std::string &d)
	:id(i),name(n),creator(c),description(d){}
	Chat(const std::string &n,const std::string &c,const std::string &d)
	:id(0),name(n),creator(c),description(d){}
	bool operator==(const Chat &other)const{
		return id==other.id&&name==other.name&&creator==other.creator&&description==other.description;
	}

	unsigned long long id;
	std::string name;
	std::string creator;
	std::string description;
};

#endif // CHAT_H
