#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

#include "network.h"
#include "ChatWorkUnit.h"
#include "Database.h"

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
	explicit ChatService(const std::string&);
	~ChatService();
	void add_work(const ChatWorkUnit*);
	void operator()();
	void send(const void*,int);
	void recv(void*,int);
	void send_string(const std::string&);
	std::string get_string();
	bool is_connected()const;

private:
	void loop();
	void recv_server_cmd();
	void heartbeat();
	void reconnect();
	const ChatWorkUnit *get_work();
	void process_connect(const ChatWorkUnitConnect&);
	void process_list_chats(const ChatWorkUnitListChats&);
	void process_newchat(const ChatWorkUnitNewChat&);
	void process_subscribe(const ChatWorkUnitSubscribe&);
	void process_send_message(const ChatWorkUnitMessage&);
	void process_get_file(const ChatWorkUnitGetFile&);

	// net commands implementing ClientCommand::*
	void clientcmd_introduce();
	void clientcmd_list_chats();
	void clientcmd_new_chat(const std::string&,const std::string&);
	void clientcmd_subscribe(const std::string&,unsigned long long);
	void clientcmd_message(const Message&);
	void clientcmd_get_file(unsigned long long);
	void clientcmd_heartbeat();
	// net commands implementing ServerCommand::*
	void servercmd_introduce();
	void servercmd_list_chats();
	void servercmd_new_chat();
	void servercmd_subscribe();
	void servercmd_message();
	void servercmd_message_receipt();
	void servercmd_send_file();

	// registered callbacks
	struct{
		// called on successful connect
		std::function<void(bool,const std::string&)> connect;
		// called on chat list receipt
		std::function<void(std::vector<Chat>)> chatlist;
		// called on successful new chat
		std::function<void(bool)> newchat;
		// called on successful subscribe
		std::function<void(bool,std::vector<Message>)> subscribe;
		// called when message received
		std::function<void(Message)> message;
		// called when server sends message receipt
		std::function<void(bool,const std::string&)> receipt;
		// called when file is received from server
		std::function<void(const unsigned char*,int)> file;
	}callback;

	Database db;
	net::tcp tcp;
	std::string target; // network address of server
	std::string servername; // name of current server that this is connected to
	std::string name; // user's name
	std::string chatname; // subscribed chat
	std::atomic<bool> working; // service thread currently running
	std::atomic<bool> connected; // currently connected to server
	std::atomic<int> work_unit_count; // atomically accessible version of units.size()
	std::queue<const ChatWorkUnit*> units;
	std::mutex mutex; // guards access to <units>
	time_t last_heartbeat;
	std::thread handle;
};

#endif // CHATSERVICE_H
