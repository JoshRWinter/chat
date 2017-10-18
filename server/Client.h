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
	void join();
	const std::string &get_name()const;
	bool dead()const;
	void kick(const std::string&)const;

private:
	void send(const void*,unsigned);
	void recv(void*,unsigned);
	void loop();
	void recv_command();
	void heartbeat();
	bool subscribe(unsigned long long,const std::string&);
	std::string get_string();
	void send_string(const std::string&);

	// net commands implementing ClientCommand::*
	void clientcmd_introduce();
	void clientcmd_list_chats();
	void clientcmd_newchat();
	void clientcmd_subscribe();
	void clientcmd_message();
	// net commands implementing ServerCommand::*
	void servercmd_list_chats(const std::vector<Chat>&);
	void servercmd_new_chat(bool);
	void servercmd_subscribe(bool,unsigned long long);
	void servercmd_message(const Message&);
	void servercmd_heartbeat();

	Server &parent;
	net::tcp tcp;
	std::atomic<bool> disconnected;
	time_t last_heartbeat;
	std::thread thread;
	std::string name;
	Chat subscribed;
};

#endif // CLIENT_H
