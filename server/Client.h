#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <atomic>
#include <thread>

class Client;
class Server;

#include "../network.h"
#include "Server.h"

class Client{
public:
	explicit Client(const Server&,int);
	Client(const Client&)=delete;
	void operator=(const Client&)=delete;
	void operator()();
	std::thread &get_thread();
	const std::string &get_name()const;
	bool dead()const;

private:
	bool send(const void*,unsigned);
	bool recv(void*,unsigned);
	void disconnect();

	const Server &parent;
	std::atomic<bool> disconnected;
	std::thread thread;
	std::string name;
	net::tcp tcp;
};

#endif // CLIENT_H
