#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <mutex>
#include <exception>
#include <vector>
#include <functional>

class Database;

#include "../sqlite3.h"
#include "../chat.h"

class DatabaseException:public std::exception{
public:
	explicit DatabaseException(const std::string &m="unknown database error")
	:msg(std::string("database error: ")+m){}
	const char *what()const noexcept{
		return msg.c_str();
	}
private:
	std::string msg;
};

class Database{
public:
	explicit Database(const std::string&);
	Database(const Database&)=delete;
	~Database();
	Database &operator=(const Database)=delete;
	std::vector<Chat> get_chats();

private:
	bool exists(const std::string&)const;

	std::mutex mutex;
	sqlite3 *conn;
};

#endif // DATABASE_H
