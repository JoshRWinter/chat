#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <mutex>
#include <exception>
#include <vector>
#include <functional>
#include <unordered_map>

#define SERVER_NAME_LENGTH 25 // characters

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

	const std::string &get_name();
	const std::vector<Chat> &get_chats();
	void new_chat(const Chat&);
	unsigned long long new_msg(const Chat&,const Message&);
	std::vector<Message> get_messages_since(unsigned long long, int);
	std::vector<unsigned char> get_file(unsigned long long, int);

private:
	void initialize();
	sqlite3 *get(int) const;
	void save();

	static bool exists(const std::string&);
	static std::string gen_name();
	static std::string serialize(const Chat&);
	static Chat deserialize(const std::string&);
	static int highest_id(const std::vector<Chat>&);

	std::string unique_name;
	std::vector<Chat> list;
	std::unordered_map<int, sqlite3*> dbs;
	const std::string &db_path;
};

#endif // DATABASE_H
