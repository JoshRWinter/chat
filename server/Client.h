#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <atomic>
#include <thread>
#include <ctime>
#include <exception>
#include <mutex>
#include <queue>
#include <optional>

class Client;
class Server;

#include "network.h"
#include "Server.h"
#include "../chat.h"

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
	bool is_subscribed(const Chat&);
	void kick(const std::string&)const;
	void addmsg(const Message&);

private:
	void send(const void*,unsigned);
	void recv(void*,unsigned);
	void loop();
	void dispatch();
	void recv_command();
	void heartbeat();
	void check_timeout();
	bool subscribe(const std::string&);
	std::string get_string();
	void send_string(const std::string&);
	static std::string format(int);
	static std::string strip_new_lines(const std::string&);

	// net commands implementing ClientCommand::*
	void clientcmd_introduce();
	void clientcmd_list_chats();
	void clientcmd_newchat();
	void clientcmd_subscribe();
	void clientcmd_message();
	void clientcmd_get_file();
	// net commands implementing ServerCommand::*
	void servercmd_introduce();
	void servercmd_list_chats(const std::vector<Chat>&);
	void servercmd_new_chat(bool);
	void servercmd_subscribe(bool,unsigned long long);
	void servercmd_message(const Message&);
	void servercmd_message_receipt(bool, const std::string&);
	void servercmd_send_file(const std::vector<unsigned char>&);
	void servercmd_heartbeat();

	Server &parent;
	net::tcp tcp;
	std::atomic<bool> disconnected;
	std::queue<Message> out_queue; // pending messages to be sent
	std::atomic<int> out_queue_len; // lock free length of out_queue
	std::mutex out_queue_lock; // guards access to <out_queue>
	time_t last_sent_heartbeat;
	time_t last_received_heartbeat;
	std::string name; // client name
	std::thread thread;
	std::optional<Chat> subscribed; // current subscribed chat
};

#endif // CLIENT_H
