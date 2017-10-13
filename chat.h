#ifndef MCHAT_H
#define MCHAT_H

#include <cstdint>

#define CHAT_PORT 28859

// command from the server
enum class ServerCommand:std::uint8_t{
	HEARTBEAT,
	MESSAGE
};

// command from the client
enum class ClientCommand:std::uint8_t{
	MESSAGE,
	SUBSCRIBE,
	NEW_CHAT
};

struct Chat{
	Chat(long long i,const std::string &n,const std::string &c,const std::string &d)
	:id(i),name(n),creator(c),description(d){}

	const long long id;
	const std::string name;
	const std::string creator;
	const std::string description;
};

#endif // MCHAT_H
