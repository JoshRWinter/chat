#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <mutex>
#include <vector>
#include <exception>

#include "network.h"
#include "Client.h"
#include "Database.h"
#include "../chat.h"

class ServerException:public std::exception{
public:
	explicit ServerException(const std::string &m):msg(m){}
	const char *what()const noexcept{
		return msg.c_str();
	}
private:
	std::string msg;
};

class Server{
public:
	Server(unsigned short,const std::string&);
	Server(const Server&)=delete;
	~Server();
	void operator=(const Server&)=delete;
	void accept();
	bool running()const;
	std::vector<Chat> get_chats();
	void new_chat(const Chat&);
	void new_msg(const Chat&,const Message&);
	std::vector<Message> get_messages_since(unsigned long long,const std::string&);
	bool valid_table_name(const std::string&);
	static void sleep();

private:
	void new_client(int);

	std::atomic<bool> good;
	std::vector<std::unique_ptr<Client>> client_list;
	std::vector<Chat> chats;
	std::mutex mutex;
	net::tcp_server tcp;
	Database db;
};

#endif // SERVER_H
