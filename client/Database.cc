#include <fstream>
#include <stdexcept>

#include "../chat.h"
#include "log.h"
#include "Database.h"

Database::Database(const std::string &dbpath){
	bool create=!Database::file_exists(dbpath);

	db.open(dbpath);

	if(create){
		const std::string create_messages_table =
		"create table messages ("
		"servername text not null," // the name of the server this messages belongs to
		"chatname text not null," // the name of the chat on this server
		"id int not null," // the server controlled id of the message
		"type int not null," // the type of message (chat.h) enum MessageType
		"unixtime int not null," // timestamp of message
		"message text not null," // the message text
		"name text not null," // sender name
		"raw blob" // any raw component (image, file)
		");";

		db.execute(create_messages_table);
	}
}

void Database::set_servername(const std::string &name){
	servername=name;
}

void Database::newmsg(const Message &msg, const std::string &chatname){
	if(servername=="")
		throw std::runtime_error(DB_ERRMSG("server name not set!"));

	const std::string insert =
	"insert into messages values"
	"(?,?,?,?,?,?,?,?);";
	lite3::statement statement(db, insert);

	statement.bind(1, servername);
	statement.bind(2, chatname);
	statement.bind(3, static_cast<int>(msg.id));
	statement.bind(4, static_cast<int>(msg.type));
	statement.bind(5, static_cast<int>(msg.unixtime));
	statement.bind(6, msg.msg);
	statement.bind(7, msg.sender);
	statement.bind(8, msg.raw, msg.raw_size);

	statement.execute();
}

// get messages in the chat
// return empty list if chat doesn't exist
std::vector<Message> Database::get_msgs(const std::string &chatname){
	if(servername=="")
		throw std::runtime_error(DB_ERRMSG("server name is not set!"));

	const std::string query =
	"select * from messages "
	"where servername=? and chatname=?;";
	lite3::statement statement(db, query);

	statement.bind(1, servername);
	statement.bind(2, chatname);

	std::vector<Message> msgs;

	while(statement.execute()){
		// get the raw component
		const int raw_size = statement.blob_size(7);
		const unsigned char *const r = (unsigned char*)statement.blob(7);
		unsigned char *raw = NULL;
		// have to copy it from sqlite's memory
		if(r != NULL){
			raw = new unsigned char[raw_size];
			memcpy(raw, r, raw_size);
		}

		msgs.push_back({
			(unsigned long long)statement.integer(2),
			static_cast<MessageType>(statement.integer(3)),
			statement.integer(4),
			statement.str(5),
			statement.str(6),
			raw,
			(decltype(Message::raw_size))raw_size
		});
	}

	return msgs;
}

// get the id of the latest message received in chat <chatname>, return 0 if no <chatname>
int Database::get_latest_msg(const std::string &chatname){
	if(servername=="")
		throw std::runtime_error(DB_ERRMSG("servername not set!"));

	const std::string query =
	"select max(id) from messages where servername=? and chatname=?;";
	lite3::statement statement(db, query);

	statement.bind(1, servername);
	statement.bind(2, chatname);

	if(!statement.execute())
		return 0;
	else
		return statement.integer(0);
}

// see if file exists
bool Database::file_exists(const std::string &dbpath){
	std::ifstream ifs(dbpath);
	return !!ifs;
}
