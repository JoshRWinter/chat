#ifndef MCHAT_H
#define MCHAT_H

#include <cstdint>

#define CHAT_PORT 28859

enum class ServerCommand:std::uint8_t{
	HEARTBEAT
};

enum class ClientCommand:std::uint8_t{
	MESSAGE
};

struct Chat{
	Chat(int i,const std::string &n,const std::string &c,const std::string &d)
	:id(i),name(n),creator(c),description(d){}

	const int id;
	const std::string name;
	const std::string creator;
	const std::string description;
};

#endif // MCHAT_H
