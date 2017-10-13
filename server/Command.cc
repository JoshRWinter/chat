#include <vector>

#include "Command.h"
#include "../chat.h"

static std::string get_string(Client &client){
	// get the size
	std::uint32_t size;
	client.recv(&size,sizeof(size));

	// get <size> characters
	std::vector<char> raw(size+1);
	client.recv(&raw[0],size);
	raw[size]=0;

	return {&raw[0]};
}

static void send_string(Client &client,const std::string &str){
	// send the size
	std::uint32_t size=str.length();
	client.send(&size,sizeof(size));

	// send the string
	client.send(str.c_str(),size);
}

namespace Command{

// send the client a heartbeat to see if they are disconnected
void send_heartbeat(Client &client){
	ServerCommand type=ServerCommand::HEARTBEAT;

	client.send(&type,sizeof(type));
}

// server allows client to create new chats
Chat recv_newchat(Client &client){
	// recv the id of the chat
	std::int64_t id;
	client.recv(&id,sizeof(id));

	// recv the name of the chat
	const std::string name=get_string(client);

	// recv the creator of the chat
	const std::string creator=get_string(client);

	// recv the description
	const std::string description=get_string(client);

	return {id,name,creator,description};
}

// client is sending a message
std::string recv_message(Client &client){
	return get_string(client);
}

// recv clients name
std::string recv_name(Client &client){
	return get_string(client);
}

} // namespace Command
