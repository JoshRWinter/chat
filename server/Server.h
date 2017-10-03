#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <mutex>
#include <vector>

#include "../network.h"
#include "Client.h"

class Server{
public:
	explicit Server(unsigned short);
	Server(const Server&)=delete;
	~Server();
	bool operator!()const;
	void operator=(const Server&)=delete;
	void accept();
	bool running()const;

private:
	void new_client(int);

	std::atomic<bool> good;
	std::vector<std::unique_ptr<Client>> client_list;
	net::tcp_server tcp;
};

#endif // SERVER_H
