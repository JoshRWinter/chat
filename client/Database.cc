#include <fstream>

#include "../chat.h"
#include "log.h"
#include "Database.h"

Database::Database(const std::string &dbpath){
	bool create=!Database::file_exists(dbpath);

	if(sqlite3_open(dbpath.c_str(),&conn)){
		const std::string msg=sqlite3_errmsg(conn);
		sqlite3_close(conn);
		conn=NULL;
		throw DatabaseException(DB_ERRMSG(msg));
	}

	if(create){
		sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);

		// create the server table
		const char *srvrtable=
		"create table servers(\n"
		"name varchar(25) not null);";

		sqlite3_stmt *stmt;
		sqlite3_prepare_v2(conn,srvrtable,-1,&stmt,NULL);

		if(sqlite3_step(stmt)!=SQLITE_DONE){
			sqlite3_exec(conn,"ROLLBACK",NULL,NULL,NULL);
			sqlite3_finalize(stmt);
			throw DatabaseException(DB_ERRMSG(sqlite3_errmsg(conn)));
		}

		sqlite3_finalize(stmt);

		// create the chats table
		const char *chatstable=
		"create table chats(\n"
		"id integer primary key autoincrement,"
		"name varchar(511) not null,"
		"servername varchar(25) not null);";

		sqlite3_prepare_v2(conn,chatstable,-1,&stmt,NULL);

		if(sqlite3_step(stmt)!=SQLITE_DONE){
			sqlite3_exec(conn,"ROLLBACK",NULL,NULL,NULL);
			sqlite3_finalize(stmt);
			throw DatabaseException(DB_ERRMSG(sqlite3_errmsg(conn)));
		}

		sqlite3_finalize(stmt);
		sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
	}
}

Database::~Database(){
	sqlite3_close(conn);
}

void Database::set_servername(const std::string &name){
	servername=name;

	const char *query=
	"select * from servers where name = ?;";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,query,-1,&statement,NULL);

	sqlite3_bind_text(statement,1,name.c_str(),-1,SQLITE_TRANSIENT);

	bool exists=sqlite3_step(statement)==SQLITE_ROW;

	if(exists&&sqlite3_step(statement)!=SQLITE_DONE){
		sqlite3_finalize(statement);
		throw DatabaseException(DB_ERRMSG(std::string("more than one entry for servername=")+servername));
	}

	sqlite3_finalize(statement);

	if(!exists){
		// insert it
		const char *insert=
		"insert into servers values\n"
		"(?);";

		sqlite3_prepare_v2(conn,insert,-1,&statement,NULL);

		sqlite3_bind_text(statement,1,name.c_str(),-1,SQLITE_TRANSIENT);

		if(sqlite3_step(statement)!=SQLITE_DONE){
			sqlite3_finalize(statement);
			throw DatabaseException(DB_ERRMSG(sqlite3_errmsg(conn)));
		}

		sqlite3_finalize(statement);
	}
}

// see if <chatname> exists in the database
bool Database::chat_exists(const std::string &chatname){
	if(servername=="")
		throw DatabaseException(DB_ERRMSG("server name is not set"));

	const char *query=
	"select * from chats where name = ? and servername = ?;";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,query,-1,&statement,NULL);

	sqlite3_bind_text(statement,1,chatname.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(statement,2,servername.c_str(),-1,SQLITE_TRANSIENT);

	bool exists=false;
	const int rc=sqlite3_step(statement);
	if(rc==SQLITE_ERROR){
		sqlite3_finalize(statement);
		throw DatabaseException(DB_ERRMSG(sqlite3_errmsg(conn)));
	}
	else if(rc==SQLITE_ROW)
		exists=true;

	sqlite3_finalize(statement);

	return exists;
}

// insert a new chat into the database, if it does not already exist
void Database::newchat(const std::string &chatname){
	if(servername=="")
		throw DatabaseException(DB_ERRMSG("server name is not set"));

	// see if chat exists in chats table
	bool exists=chat_exists(chatname);

	if(!exists){
		// insert it
		sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);

		const char *insert=
		"insert into chats(name, servername) values\n"
		"(?,?);";

		sqlite3_stmt *statement;
		sqlite3_prepare_v2(conn,insert,-1,&statement,NULL);

		sqlite3_bind_text(statement,1,chatname.c_str(),-1,SQLITE_TRANSIENT);
		sqlite3_bind_text(statement,2,servername.c_str(),-1,SQLITE_TRANSIENT);

		if(sqlite3_step(statement)!=SQLITE_DONE){
			sqlite3_finalize(statement);
			sqlite3_exec(conn,"ROLLBACK",NULL,NULL,NULL);
			throw DatabaseException(DB_ERRMSG(sqlite3_errmsg(conn)));
		}

		sqlite3_finalize(statement);

		// make a chat table for this chat
		const std::string fqchatname=chatname+"$"+servername;
		const std::string maketable=std::string("")+
		"create table "+Database::escape_table_name(fqchatname)+"(\n"
		"id integer primary key,"
		"type int not null,"
		"message varchar(" MESSAGE_LEN_STR ") not null,"
		"name varchar(511) not null,"
		"raw blob);";

		if(sqlite3_prepare(conn,maketable.c_str(),-1,&statement,NULL)){
			log_error(std::string("poooooooooooooooop ")+sqlite3_errmsg(conn));
		}


		int rc;
		if((rc=sqlite3_step(statement))!=SQLITE_DONE){
			log_error(sqlite3_errstr(rc));
			sqlite3_finalize(statement);
			sqlite3_exec(conn,"ROLLBACK",NULL,NULL,NULL);
			throw DatabaseException(DB_ERRMSG(sqlite3_errmsg(conn)));
		}

		sqlite3_finalize(statement);

		sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
	}
}

void Database::newmsg(const Message &msg,const std::string &chatname){
	if(servername=="")
		throw DatabaseException(DB_ERRMSG("server name not set!"));

	const std::string fqchatname=chatname+"$"+servername;
	const std::string insert=std::string("")+
	"insert into "+Database::escape_table_name(fqchatname)+" (type,message,name) values\n"
	"(?,?,?);";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,insert.c_str(),-1,&statement,NULL);

	sqlite3_bind_int(statement,1,msg.id);
	sqlite3_bind_text(statement,2,msg.msg.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(statement,3,msg.sender.c_str(),-1,SQLITE_TRANSIENT);

	if(sqlite3_step(statement)!=SQLITE_DONE){
		sqlite3_finalize(statement);
		throw DatabaseException(DB_ERRMSG(sqlite3_errmsg(conn)));
	}

	sqlite3_finalize(statement);
}

// get messages in the chat
// return empty list if chat doesn't exist
std::vector<Message> Database::get_msgs(const std::string &chatname){
	if(servername=="")
		throw DatabaseException(DB_ERRMSG("server name is not set!"));

	if(!chat_exists(chatname))
		return {};

	const std::string fqchatname=chatname+"$"+servername;
	const std::string query=std::string("")+
	"select * from "+Database::escape_table_name(fqchatname)+";";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,query.c_str(),-1,&statement,NULL);

	std::vector<Message> msgs;
	int rc;
	while((rc=sqlite3_step(statement))!=SQLITE_DONE){
		if(rc==SQLITE_ERROR){
			sqlite3_finalize(statement);
			throw DatabaseException(DB_ERRMSG(sqlite3_errmsg(conn)));
		}

		msgs.push_back({
			(unsigned long long)sqlite3_column_int(statement,0),
			static_cast<MessageType>(sqlite3_column_int(statement,1)),
			(char*)sqlite3_column_text(statement,2),
			(char*)sqlite3_column_text(statement,3),
			NULL,
			0
		});
	}

	sqlite3_finalize(statement);

	return msgs;
}

// get the id of the latest message received in chat <chatname>, return 0 if no <chatname>
int Database::get_latest_msg(const std::string &chatname){
	if(servername=="")
		throw DatabaseException(DB_ERRMSG("servername not set!"));

	if(!chat_exists(chatname))
		return 0;

	const std::string fqchatname=chatname+"$"+servername;
	const std::string query=std::string("")+
	"select max(id) from "+Database::escape_table_name(fqchatname)+";";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,query.c_str(),-1,&statement,NULL);

	int max=0;
	const int rc=sqlite3_step(statement);
	if(rc==SQLITE_ERROR){
		sqlite3_finalize(statement);
		throw DatabaseException(DB_ERRMSG(sqlite3_errmsg(conn)));
	}
	if(rc==SQLITE_ROW)
		max=sqlite3_column_int(statement,0);

	sqlite3_finalize(statement);

	return max;
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

// see if file exists
bool Database::file_exists(const std::string &dbpath){
	std::ifstream ifs(dbpath);
	return !!ifs;
}
