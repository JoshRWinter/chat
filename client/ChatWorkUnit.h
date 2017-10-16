#ifndef CHATWORKUNIT_H
#define CHATWORKUNIT_h

#include <variant>
#include <functional>

#include "../chat.h"

enum class WorkUnitType:std::uint8_t{
	CONNECT, // connect to a server
	NEW_CHAT, // have the server make a new chat
	SUBSCRIBE, // subscribe to a chat
	MESSAGE // send a message
};

struct ChatWorkUnit{
	WorkUnitType type;

	struct connect{ // if WorkUnitType::CONNECT
		std::string target;
		std::string myname;
		std::function<void(bool,std::vector<Chat>)> callback;
	};

	struct newchat{ // if WorkUnitType::NEW_CHAT
		std::string chatname;
		std::function<void(bool)> callback;
	};

	struct subscribe{ // if WorkUnitType::SUBSCRIBE
		int id;
		std::function<void(bool)> callback;
		std::function<void(Message)> msgcallback;
	};

	Message msg;

	std::variant<ChatWorkUnit::connect, ChatWorkUnit::newchat, ChatWorkUnit::subscribe, Message> work;
};

#endif // CHATWORKUNIT_H
