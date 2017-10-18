#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <thread>
#include <atomic>
#include <mutex>
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
	void add_work(const ChatWorkUnit*);
	void operator()();
	void send(const void*,int);
	void recv(void*,int);
	void send_string(const std::string&);
	std::string get_string();

private:
	void loop();
	void recv_server_cmd();
	void reconnect();
	const ChatWorkUnit *get_work();
	void process_connect(const ChatWorkUnit &unit);
	void process_newchat(const ChatWorkUnit &unit);
	void process_subscribe(const ChatWorkUnit &unit);
	void process_send(const ChatWorkUnit &unit);

	// net commands implementing ClientCommand::*
	void clientcmd_introduce();
	void clientcmd_list_chats();
	void clientcmd_new_chat(const Chat&);
	void clientcmd_subscribe(const Chat&);
	void clientcmd_message(const Message&);
	// net commands implementing ServerCommand::*
	void servercmd_list_chats();
	void servercmd_new_chat();
	void servercmd_subscribe();
	void servercmd_message();

	// registered callbacks
	struct{
		// called on successful connect
		std::function<void(bool,std::vector<Chat>)> connect;
		// called on successful new chat
		std::function<void(bool)> newchat;
		// called on successful subscribe
		std::function<void(bool,std::vector<Message>)> subscribe;
		// called when message received
		std::function<void(Message)> message;
	}callback;

	net::tcp tcp;
	std::string name; // user's name
	std::atomic<bool> working; // service thread currently running
	std::atomic<int> work_unit_count; // atomically accessible version of units.size()
	std::queue<const ChatWorkUnit*> units;
	std::mutex mutex; // guards access to <units>
	std::thread handle;
	std::function<void(Message)> msg_callback;

};

#endif // CHATSERVICE_H
