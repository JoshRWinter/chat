#include <fstream>

#include "Database.h"
#include "log.h"

Database::Database(const std::string &fname){
	bool create=!exists(fname);

	if(sqlite3_open(fname.c_str(),&conn)){
		std::string message(sqlite3_errmsg(conn));
		sqlite3_close(conn);
		throw DatabaseException(message);
	}

	if(create){
		// create the main chats table
		const char *stmt=
		"create table chats(\n"
		"id integer primary key autoincrement,\n"
		"name varchar(255) not null,\n"
		"creator varchar(255) not null,\n"
		"description varchar(511) not null\n"
		");";

		char *err;
		if(sqlite3_exec(conn,stmt,NULL,NULL,&err)){
			std::string msg(err);
			sqlite3_free(err);
			throw DatabaseException(msg);
		}
	}
}

Database::~Database(){
	sqlite3_close(conn);
}

// return a vector of all the chats currently registered in the database
std::vector<Chat> Database::get_chats(){
	const char *query=
	"select * from chats;";
	std::vector<Chat> chats;

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,query,-1,&statement,NULL);

	int rc;
	while((rc=sqlite3_step(statement))!=SQLITE_DONE){
		if(rc!=SQLITE_ROW)
			throw DatabaseException(sqlite3_errmsg(conn));

		chats.push_back({
			(unsigned long long)sqlite3_column_int64(statement,0),
			(char*)sqlite3_column_text(statement,1),
			(char*)sqlite3_column_text(statement,2),
			(char*)sqlite3_column_text(statement,3)
		});
	}

	sqlite3_finalize(statement);

	return chats;
}

// register a new chat to the database
void Database::new_chat(const Chat &chat){
	sqlite3_exec(conn,"BEGIN TRANSACTION",NULL,NULL,NULL);
	// now create a table for the messages
	const std::string newtable=std::string("")+
	"create table "+Database::escape_table_name(chat.name)+" (\n"
	"id integer primary key autoincrement,\n"
	"type int not null,\n" // MessageType enum in chat.h
	"message varchar(" MESSAGE_LEN_STR ") not null,\n"
	"name varchar(511) not null,\n"
	"raw blob);"; // reserved for file content, image content, will be null for normal messages

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,newtable.c_str(),-1,&statement,NULL);

	sqlite3_bind_text(statement,1,chat.name.c_str(),-1,SQLITE_TRANSIENT);

	if(sqlite3_step(statement)!=SQLITE_DONE){
		sqlite3_exec(conn,"ROLLBACK",NULL,NULL,NULL);
		throw DatabaseException(std::string("couldn't create new table for chat \"")+chat.name+"\": "+sqlite3_errmsg(conn));
	}

	sqlite3_finalize(statement);

	const char *sqlstatement=
	"insert into chats(name,creator,description) values\n"
	"(?,?,?)";

	sqlite3_prepare_v2(conn,sqlstatement,-1,&statement,NULL);

	sqlite3_bind_text(statement,1,chat.name.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(statement,2,chat.creator.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(statement,3,chat.description.c_str(),-1,SQLITE_TRANSIENT);

	if(sqlite3_step(statement)!=SQLITE_DONE){
		sqlite3_exec(conn,"ROLLBACK",NULL,NULL,NULL);
		throw DatabaseException(std::string("couldn't add new chat to database: ")+sqlite3_errmsg(conn));
	}

	sqlite3_finalize(statement);
	sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
}

// get all messages from chat <name> where id is bigger than <since>
std::vector<Message> Database::get_messages_since(unsigned long long since,const std::string &name){
	const std::string query=std::string("")+
	"select * from "+escape_table_name(name)+" where id > ?;";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,query.c_str(),-1,&statement,NULL);

	sqlite3_bind_int64(statement,1,since);

	std::vector<Message> messages;
	int rc;
	while((rc=sqlite3_step(statement))!=SQLITE_DONE){
		if(rc!=SQLITE_ROW)
			throw DatabaseException(sqlite3_errmsg(conn));

		messages.push_back({
			(decltype(Message::id))sqlite3_column_int(statement,0),
			static_cast<MessageType>(sqlite3_column_int(statement,1)),
			(char*)sqlite3_column_text(statement,2),
			(char*)sqlite3_column_text(statement,3),
			NULL,
			0
		});
	}

	sqlite3_finalize(statement);

	return messages;
}

// see if table name is valid
bool Database::valid_table_name(const std::string &name){
	// sqlite table names are not allowed to start with "sqlite_..."
	if(name.find("sqlite_")==0)
		return false;

	// reject empty name
	if(name.length()==0)
		return false;

	// make sure no chat already named <name>
	const char *query=
	"select * from chats where name = ?";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,query,-1,&statement,NULL);

	sqlite3_bind_text(statement,1,name.c_str(),-1,SQLITE_TRANSIENT);

	bool exists=sqlite3_step(statement)==SQLITE_ROW;
	sqlite3_finalize(statement);

	return !exists;
}

// return true if the database exists and does not need to be created
bool Database::exists(const std::string &fname)const{
	std::ifstream file(fname);
	return !!file;
}

std::string Database::escape_table_name(const std::string &name){
	std::string escaped=std::string("\"")+name+"\"";

	for(unsigned i=1;i<escaped.length()-1;++i){
		if(escaped[i]=='"'){
			escaped.insert(i,"\"");
			++i;
		}
	}

	return escaped;
}
