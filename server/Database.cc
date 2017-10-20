#include <fstream>
#include <cstdlib>

#include <time.h>

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
		// create a random server name
		std::srand(time(NULL));
		char servername[26];
		for(int i=0;i<25;++i){
			char c='A'+(std::rand()%26);
			servername[i]=c;
		}
		servername[25]=0;

		// store it in the db
		char *err=NULL;
		const char *makesrvrname=
		"create table servername(\n"
		"name varchar(25) not null);";
		if(sqlite3_exec(conn,makesrvrname,NULL,NULL,&err)){
			const std::string msg(err);
			sqlite3_free(err);
			throw DatabaseException(msg);
		}
		const std::string storename=std::string("")+
		"insert into servername values\n"
		"('"+servername+"');";
		if(sqlite3_exec(conn,storename.c_str(),NULL,NULL,&err)){
			const std::string msg(err);
			sqlite3_free(err);
			throw DatabaseException(msg);
		}

		// create the main chats table
		const char *stmt=
		"create table chats(\n"
		"id integer primary key autoincrement,\n"
		"name varchar(255) not null,\n"
		"creator varchar(255) not null,\n"
		"description varchar(511) not null\n"
		");";

		if(sqlite3_exec(conn,stmt,NULL,NULL,&err)){
			const std::string msg(err);
			sqlite3_free(err);
			throw DatabaseException(msg);
		}
	}
}

Database::~Database(){
	sqlite3_close(conn);
}

// get the name of the server
std::string Database::get_name(){
	const char *query=
	"select name from servername;";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,query,-1,&statement,NULL);

	if(sqlite3_step(statement)!=SQLITE_ROW){
		sqlite3_finalize(statement);
		throw DatabaseException(sqlite3_errmsg(conn));
	}

	std::string dbname=(char*)sqlite3_column_text(statement,0);

	if(sqlite3_step(statement)!=SQLITE_DONE){
		sqlite3_finalize(statement);
		throw DatabaseException("more than one record in servername table!");
	}

	sqlite3_finalize(statement);

	return dbname;
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
		if(rc!=SQLITE_ROW){
			sqlite3_finalize(statement);
			throw DatabaseException(sqlite3_errmsg(conn));
		}

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
	sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
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
		sqlite3_finalize(statement);
		throw DatabaseException(std::string("couldn't create new table for chat \"")+chat.name+"\": "+sqlite3_errmsg(conn));
	}

	sqlite3_finalize(statement);

	const char *sqlstatement=
	"insert into chats(name,creator,description) values\n"
	"(?,?,?);";

	sqlite3_prepare_v2(conn,sqlstatement,-1,&statement,NULL);

	sqlite3_bind_text(statement,1,chat.name.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(statement,2,chat.creator.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(statement,3,chat.description.c_str(),-1,SQLITE_TRANSIENT);

	if(sqlite3_step(statement)!=SQLITE_DONE){
		sqlite3_exec(conn,"ROLLBACK",NULL,NULL,NULL);
		sqlite3_finalize(statement);
		throw DatabaseException(std::string("couldn't add new chat to database: ")+sqlite3_errmsg(conn));
	}

	sqlite3_finalize(statement);
	sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
}

// insert a new message into database
unsigned long long Database::new_msg(const Chat &chat,const Message &msg){
	std::string insert=std::string("")+
	"insert into "+Database::escape_table_name(chat.name)+" (type,message,name,raw) values\n"
	"(?,?,?,?);";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,insert.c_str(),-1,&statement,NULL);

	sqlite3_bind_int(statement,1,static_cast<int>(msg.type));
	sqlite3_bind_text(statement,2,msg.msg.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(statement,3,msg.sender.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_blob(statement,4,msg.raw,msg.raw_size,SQLITE_TRANSIENT);

	if(sqlite3_step(statement)!=SQLITE_DONE){
		sqlite3_finalize(statement);
		throw DatabaseException(sqlite3_errmsg(conn));
	}

	sqlite3_finalize(statement);

	// get id of last inserted item
	const std::string query=std::string("")+
	"select max(id) from "+Database::escape_table_name(chat.name)+";";
	sqlite3_prepare_v2(conn,query.c_str(),-1,&statement,NULL);

	if(sqlite3_step(statement)!=SQLITE_ROW){
		sqlite3_finalize(statement);
		throw DatabaseException(sqlite3_errmsg(conn));
	}

	unsigned id=sqlite3_column_int(statement,0);

	sqlite3_finalize(statement);

	return id;
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
		if(rc!=SQLITE_ROW){
			sqlite3_finalize(statement);
			throw DatabaseException(sqlite3_errmsg(conn));
		}

		// get the blob first
		const int raw_size=sqlite3_column_bytes(statement,4);
		const unsigned char *r=(unsigned char*)sqlite3_column_blob(statement,4);
		unsigned char *raw=NULL;
		if(r!=NULL){
			raw=new unsigned char[raw_size];
			memcpy(raw,r,raw_size);
		}

		messages.push_back({
			(decltype(Message::id))sqlite3_column_int(statement,0),
			static_cast<MessageType>(sqlite3_column_int(statement,1)),
			(char*)sqlite3_column_text(statement,2),
			(char*)sqlite3_column_text(statement,3),
			raw,
			(decltype(Message::raw_size))raw_size
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

	// no dollars pls
	for(char s:name)
		if(s=='$')
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
