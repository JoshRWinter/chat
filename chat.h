#ifndef CHAT_H
#define CHAT_H

#include <cstdint>
#include <memory>

#define CHAT_PORT 28859

#define MESSAGE_LEN_INT 510 // these two must
#define MESSAGE_LEN_STR "510" // be kept in sync

// command from the server
enum class ServerCommand:std::uint8_t{
	HEARTBEAT, // server is sending a heartbeat to client
	MESSAGE // server is sending a client a message
};

// command from the client
enum class ClientCommand:std::uint8_t{
	LIST_CHATS, // client wants the list of chats
	MESSAGE, // client is sending a message
	SUBSCRIBE, // client wants to subscribe to a chat
	INTRODUCE, // client wants to introduce himself
	NEW_CHAT // client wants to create a new chat
};

enum class MessageType:std::uint8_t{
	TEXT,
	IMAGE,
	FILE
};

// the message object
struct Message{
	Message(MessageType t,const std::string &m,const std::string &s,unsigned char *r=NULL)
	:type(t),msg(m),sender(s),raw(r){}
	Message(Message &&other){
		type=other.type;
		msg=other.msg;
		sender=other.sender;
		raw=other.raw;

		other.raw=NULL;
	}
	~Message(){
		delete raw;
	}

	MessageType type;
	std::string msg;
	std::string sender;
	unsigned char *raw;
};

// describes a "chat" the meta data for a group chat
struct Chat{
	Chat():id(0){}
	Chat(unsigned long long i,const std::string &n,const std::string &c,const std::string &d)
	:id(i),name(n),creator(c),description(d){}
	bool operator==(const Chat &other)const{
		return id==other.id&&name==other.name&&creator==other.creator&&description==other.description;
	}

	unsigned long long id;
	std::string name;
	std::string creator;
	std::string description;
};

#endif // CHAT_H
