#ifndef COMMAND_H
#define COMMAND_H

#include "Client.h"

namespace Command{
	void send_heartbeat(Client&);

	Chat recv_newchat(Client&);
	std::string recv_message(Client&);
	std::string recv_name(Client&);
}

#endif // COMMAND_H
