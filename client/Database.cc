#include <fstream>
#include <stdexcept>

#include "../chat.h"
#include "log.h"
#include "Database.h"

Database::Database(const std::string &dbpath){
	bool create=!Database::file_exists(dbpath);

	db.open(dbpath);

	if(create){
		db.begin();

		const std::string create_chats_table =
		"create table chats ("
		"chatname text not null,"
		"servername text not null,"
		"last_login int not null" // unix time stamp
		");";

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

		try
		{
			db.execute(create_messages_table);
			db.execute(create_chats_table);
		}
		catch(const lite3::exception &e)
		{
			db.rollback();
			throw;
		}

		db.commit();
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

	// first, log the current chat name and server name into table "chats" if it doesn't already exist
	log_chat(chatname);
	// now allow the database to regulate its own size
	regulate();

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

// insert <chatname> into the chats table if it doesn't already exist,
// and update its last_login timestamp if it does
void Database::log_chat(const std::string &chatname)
{
	const std::string query =
	"select * from chats where chatname=? and servername=?;";
	lite3::statement statement(db, query);

	statement.bind(1, chatname);
	statement.bind(2, servername);

	const bool exists = statement.execute();
	if(exists && statement.execute())
		throw std::runtime_error("more that one entry in chats table for chatname \"" + chatname + "\" and servername \"" + servername + "\"");

	if(!exists)
	{
		// insert it
		const std::string insert =
		"insert into chats values (?, ?, ?);";
		lite3::statement inserter(db, insert);

		inserter.bind(1, chatname);
		inserter.bind(2, servername);
		inserter.bind(3, time(NULL));

		inserter.execute();
	}
	else
	{
		// update its last_login timestamp
		const std::string alter =
		"update chats set last_login=? where chatname=? and servername=?;";
		lite3::statement alterer(db, alter);

		alterer.bind(1, time(NULL));
		alterer.bind(2, chatname);
		alterer.bind(3, servername);

		alterer.execute();
	}
}

// determine which chats haven't been logged into in a while
// using (Database::stale(...))
void Database::regulate()
{
	const std::string query =
	"select * from chats;";
	lite3::statement statement(db, query);

	while(statement.execute())
	{
		const std::string chatname = statement.str(0);
		const std::string server = statement.str(1);
		const int unixtime = statement.integer(2);

		if(Database::stale(unixtime))
			remove(chatname, server);
	}
}

// remove all messages from chatname and servername, and the entry from the chats table
void Database::remove(const std::string &chatname, const std::string &server)
{
	log("deleting all messages from chatname: \"" + chatname + "\", servername: \"" + server + "\"");
	// first remove all the appropriate messages
	const std::string sql =
	"delete from messages where chatname=? and servername=?;";
	lite3::statement s1(db, sql);

	s1.bind(1, chatname);
	s1.bind(2, server);

	s1.execute();

	// remove the entry from the chats table
	const std::string sql2 =
	"delete from chats where chatname=? and servername=?";
	lite3::statement s2(db, sql2);

	s2.bind(1, chatname);
	s2.bind(2, server);

	s2.execute();
}

// see if file exists
bool Database::file_exists(const std::string &dbpath){
	std::ifstream ifs(dbpath);
	return !!ifs;
}

bool Database::stale(int unixtime)
{
	const int now = time(NULL);

	const int stale_seconds = 2592000; // seconds in a month

	return now - unixtime > stale_seconds;
}
