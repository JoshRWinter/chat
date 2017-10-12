#include "Command.h"
#include "../chat.h"

namespace Command{

void send_heartbeat(Client &client){
	ServerCommand type=ServerCommand::HEARTBEAT;

	client.send(&type,sizeof(type));
}

std::string recv_message(Client &client){
	// get the message length
	std::uint32_t size;
	client.recv(&size,sizeof(size));

	// get the message
	std::vector<char> raw(size+1);
	raw[size]=0;

	return {&raw[0]};
}

}
