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
	void add_work(ChatWorkUnit*);
	void operator()();
	void send(const void*,int);
	void recv(void*,int);
	void send_string(const std::string&);
	std::string get_string();

private:
	void loop();
	void reconnect();
	std::unique_ptr<ChatWorkUnit> &&get_work();
	void process_connect(const ChatWorkUnit &unit);
	void process_newchat(const ChatWorkUnit &unit);
	void process_subscribe(const ChatWorkUnit &unit);
	void process_send(const ChatWorkUnit &unit);

	net::tcp tcp;
	std::atomic<bool> working; // service thread currently running
	std::atomic<int> work_unit_count; // atomically accessible version of units.size()
	std::queue<std::unique_ptr<ChatWorkUnit>> units;
	std::mutex mutex; // guards access to <units>
	std::thread handle;
	std::function<void(Message)> msg_callback;

};

#endif // CHATSERVICE_H
