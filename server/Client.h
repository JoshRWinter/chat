#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <atomic>
#include <thread>
#include <ctime>

class Client;
class Server;

#include "../network.h"
#include "Server.h"

#define HEARTBEAT_FREQUENCY 15

struct NetworkException{
	virtual const char *what()const{
		return "network error";
	}
};

struct ShutdownException{
	virtual const char *what()const{
		return "need to shut down";
	}
};

class Client{
public:
	explicit Client(const Server&,int);
	Client(const Client&)=delete;
	void operator=(const Client&)=delete;
	void operator()();
	std::thread &get_thread();
	const std::string &get_name()const;
	bool dead()const;
	void send(const void*,unsigned);
	void recv(void*,unsigned);

private:
	void setup();
	void loop();
	void recv_command();
	void heartbeat();
	void disconnect();

	const Server &parent;
	std::atomic<bool> disconnected;
	std::thread thread;
	std::string name;
	net::tcp tcp;
	time_t last_heartbeat;
};

#endif // CLIENT_H
