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
	Database &operator=(const Database&)=delete;
	std::vector<Chat> get_chats();
	void new_chat(const Chat&);
	void new_msg(const Chat&,const Message&);
	std::vector<Message> get_messages_since(unsigned long long,const std::string&);
	bool valid_table_name(const std::string&);

private:
	bool exists(const std::string&)const;
	static std::string escape_table_name(const std::string&);

	sqlite3 *conn;
};

#endif // DATABASE_H
