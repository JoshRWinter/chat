#ifndef MCHAT_H
#define MCHAT_H

#include <cstdint>

#define MCHAT_PORT 28859

enum class ServerCommand:std::uint8_t{
	HEARTBEAT
};

#endif // MCHAT_H
