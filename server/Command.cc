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
// implements ServerCommand::HEARTBEAT
void send_heartbeat(Client &client){
	ServerCommand type=ServerCommand::HEARTBEAT;

	client.send(&type,sizeof(type));
}

// lists the chats
// implements ClientCommand::LIST_CHATS
void send_all_chats(Client &client){
	std::vector<Chat> chats=client.parent.get_chats();

	std::uint64_t count=chats.size();
	client.send(&count,sizeof(count));

	for(const Chat &chat:chats){
		client.send(&chat.id,sizeof(chat.id));
		send_string(client,chat.name);
		send_string(client,chat.creator);
		send_string(client,chat.description);
	}
}

// client is sending a message
// implements ClientCommand::MESSAGE
Message &&recv_message(Client &client){
	// receive the msg type
	std::uint8_t type;
	client.recv(&type,sizeof(type));

	MessageType mtype=static_cast<MessageType>(type);

	switch(mtype){
	case MessageType::TEXT:
	case MessageType::FILE:
	case MessageType::IMAGE:
		break;
	default:
		// illegal
		client.kick("illegal message type received: "+std::to_string(type));
		break;
	}

	// receive the message
	std::string message=get_string(client);

	return std::move(Message(mtype,message,client.get_name(),NULL,0));
}

// allow the client to subscribe to a chat
// implements ClientCommand::SUBSCRIBE
void recv_subscription(Client &client){
	// get the id of the desired chat
	std::uint64_t cid;
	client.recv(&cid,sizeof(cid));

	// get the name of the desired chat
	std::string name=get_string(client);

	std::uint8_t success=client.subscribe(cid,name)?1:0;
	client.send(&success,sizeof(success));

	// recv the max message id in that chat
	std::uint64_t max;
	client.recv(&max,sizeof(max));

	// send all messages in the chat where message.id > max
	// basically getting the client back up to date since they were last connected
	std::vector<Message> since=client.parent.get_messages_since(max,client.subscribed.name);

	// send the count
	std::uint64_t count=since.size();

	// send <count> messages
	for(const Message &msg:since){
		// send the message type
		std::uint8_t type=static_cast<std::uint8_t>(msg.type);
		client.send(&type,sizeof(type));

		// send msg
		send_string(client,msg.msg);

		// send sender
		send_string(client,msg.sender);

		// send raw
		std::uint64_t raw_size=0;
		client.send(&raw_size,sizeof(raw_size));
	}
}

// recv clients name
// implements ClientCommand::INTRODUCE
std::string recv_name(Client &client){
	return get_string(client);
}

// server allows client to create new chats
// implements ClientCommand::NEW_CHAT
Chat recv_newchat(Client &client){
	// recv the id of the chat
	std::uint64_t id;
	client.recv(&id,sizeof(id));

	// recv the name of the chat
	std::string name=get_string(client);

	// recv the creator of the chat
	const std::string creator=get_string(client);

	// recv the description
	const std::string description=get_string(client);

	// validate the table name
	std::uint8_t valid=client.parent.valid_table_name(name)?1:0;
	// tell the client
	client.send(&valid,sizeof(valid));
	if(!valid)
		name="(not valid!)";

	return {id,name,creator,description};
}

} // namespace Command
