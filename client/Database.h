#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <exception>
#include <vector>

#include "../sqlite3.h"

#define DB_ERRMSG(x) (std::string(__FILE__)+":"+std::to_string(__LINE__)+" "+x)

struct DatabaseException:std::exception{
	DatabaseException(const std::string &m):msg(std::string("Database error: ")+m){}
	virtual const char *what()const noexcept{
		return msg.c_str();
	}
	const std::string msg;
};

class Database{
public:
	explicit Database(const std::string&);
	~Database();
	Database(const Database&)=delete;
	Database &operator=(const Database&)=delete;
	void set_servername(const std::string&);
	bool chat_exists(const std::string&);
	void newchat(const std::string&);
	void newmsg(const Message&,const std::string&);
	std::vector<Message> get_msgs(const std::string&);
	int get_latest_msg(const std::string&);

private:
	static std::string escape_table_name(const std::string&);
	static bool file_exists(const std::string&);

	std::string servername;
	sqlite3 *conn;
};

#endif // DATABASE_H
