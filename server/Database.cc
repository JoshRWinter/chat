#include <fstream>
#include <iostream>

#include "Database.h"

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
		"description varchar(511)\n"
		");";

		char *err;
		if(sqlite3_exec(conn,stmt,NULL,NULL,&err)){
			std::string msg(err);
			sqlite3_free(err);
			throw DatabaseException(msg);
		}
	}

	std::vector<Chat> chats=get_chats();
	for(Chat &chat:chats){
		std::cout<<"id: "<<chat.id<<", name: "<<chat.name<<", creator: "<<chat.creator<<", desc: "<<chat.description<<std::endl;
	}
}

Database::~Database(){
	sqlite3_close(conn);
}

std::vector<Chat> Database::get_chats(){
	std::lock_guard<std::mutex> lock(mutex);

	const char *query=
	"select * from chats;";
	std::vector<Chat> chats;

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,query,-1,&statement,NULL);

	while(sqlite3_step(statement)!=SQLITE_DONE){
		int id=sqlite3_column_int(statement,0);
		const std::string name=(char*)sqlite3_column_text(statement,1);
		const std::string creator=(char*)sqlite3_column_text(statement,2);
		const char *desc=(char*)sqlite3_column_text(statement,3);
		const std::string description(desc==NULL?"":desc);

		chats.push_back({id,name,creator,description});
	}

	return chats;
}

bool Database::exists(const std::string &fname)const{
	std::ifstream file(fname);
	return !!file;
}
