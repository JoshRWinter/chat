#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <exception>
#include <vector>

#include "lite3.hpp"

#define DB_ERRMSG(x) (std::string(__FILE__)+":"+std::to_string(__LINE__)+" "+x)

class Database{
public:
	explicit Database(const std::string&);
	Database(const Database&)=delete;

	Database &operator=(const Database&)=delete;

	void set_servername(const std::string&);
	void newmsg(const Message&,const std::string&);
	std::vector<Message> get_msgs(const std::string&);
	int get_latest_msg(const std::string&);

private:
	static std::string escape_table_name(const std::string&);
	static bool file_exists(const std::string&);

	std::string servername;
	lite3::connection db;
};

#endif // DATABASE_H
