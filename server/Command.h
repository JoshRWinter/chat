#ifndef COMMAND_H
#define COMMAND_H

#include "Client.h"

namespace Command{
	void send_heartbeat(Client&);

	void send_all_chats(Client&);
	Message &&recv_message(Client&);
	void recv_subscription(Client&);
	std::string recv_name(Client&);
	Chat recv_newchat(Client&);
}

#endif // COMMAND_H
