#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <queue>

#include "ChatWorkUnit.h"
#include "network.h"

class NetworkException:public std::exception{
public:
	virtual const char *what()const noexcept{
		return "a network error has ocurred";
	}
};

class ShutdownException:public std::exception{
public:
	virtual const char *what()const noexcept{
		return "need to shut down";
	}
};

class ChatService{
public:
	ChatService();
	~ChatService();
	void add_command(Command*);
	void operator()();

private:
	void loop();
	void reconnect();
	std::unique_ptr<Command> &&get_command();
	void send(void*,int);
	void recv(void*,int);

	net::tcp tcp;
	std::atomic<bool> working; // service thread currently running
	std::atomic<int> command_count; // atomically accessible version of commands.size()
	std::queue<std::unique_ptr<Command>> commands;
	std::mutex mutex; // guards access to <commands>
	std::thread handle;
};

#endif // CHATSERVICE_H
