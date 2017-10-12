#ifndef COMMAND_H
#define COMMAND_H

#include "Client.h"

namespace Command{
	void send_heartbeat(Client&);
	std::string recv_message(Client&);
}

#endif // COMMAND_H
