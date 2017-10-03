#include "Command.h"
#include "../mchat.h"

namespace Command{

void heartbeat(Client &client){
	ServerCommand type=ServerCommand::HEARTBEAT;

	client.send(&type,sizeof(type));
}

}
