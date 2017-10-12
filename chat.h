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

#endif // MCHAT_H
