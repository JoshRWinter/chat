#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <atomic>
#include <thread>
#include <ctime>
#include <exception>

class Client;
class Server;

#include "network.h"
#include "Server.h"
#include "../chat.h"

#define HEARTBEAT_FREQUENCY 15

struct NetworkException:std::exception{
	virtual const char *what()const noexcept{
		return "network error";
	}
};

struct ShutdownException:std::exception{
	virtual const char *what()const noexcept{
		return "need to shut down";
	}
};

struct ClientKickException:std::exception{
	ClientKickException(const std::string &r):reason(r){}
	virtual const char *what()const noexcept{
		return reason.c_str();
	}
	std::string reason;
};

class Client{
public:
	explicit Client(Server&,int);
	Client(const Client&)=delete;
	void operator=(const Client&)=delete;
	void operator()();
	std::thread &get_thread();
	const std::string &get_name()const;
	bool dead()const;
	void send(const void*,unsigned);
	void recv(void*,unsigned);
	void kick(const std::string&)const;
	bool subscribe(int,const std::string&);
	std::vector<Chat> get_chats()const;

private:
	void loop();
	void recv_command();
	void heartbeat();
	void disconnect();

	Server &parent;
	std::atomic<bool> disconnected;
	std::thread thread;
	std::string name;
	Chat subscribed;
	net::tcp tcp;
	time_t last_heartbeat;
};

#endif // CLIENT_H
