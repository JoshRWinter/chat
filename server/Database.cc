#include <fstream>
#include <cstdlib>
#include <optional>

#include <time.h>

#include "Database.h"
#include "os.h"
#include "log.h"
#include "csv.h"

Database::Database(const std::string &dbpath)
	: db_path(dbpath)
{
	os::mkdir(db_path);

	const std::string &master_file_path = dbpath + "/master";
	const std::string &directory_path = db_path + "/directory";

	// create the master file
	if(!Database::exists(master_file_path)){
		// create a random server name
		unique_name = Database::gen_name();

		// store it in the "master" file
		std::ofstream masterfile(master_file_path);
		masterfile << unique_name;
	}
	else{
		// get the unique server name from the master file
		std::ifstream masterfile(master_file_path);
		std::getline(masterfile, unique_name);
		if(unique_name.size() != 25)
			throw DatabaseException("Error, server name not 25 characters");
	}

	// create the directory file
	if(!Database::exists(directory_path)){
		// create the directory file
		std::ofstream dirfile(directory_path);
	}

	// populate the chat list and dbs map
	initialize();
}

Database::~Database(){
	for(const auto &item : dbs)
		sqlite3_close(item.second);
}

// get the name of the server
const std::string &Database::get_name(){
	return unique_name;
}

// return a vector of all the chats currently registered in the database
const std::vector<Chat> &Database::get_chats(){
	return list;
}

// register a new chat to the database
void Database::new_chat(const Chat &chat){
	const int id = Database::highest_id(list) + 1;
	const std::string &path = (db_path + "/" + std::to_string(id));
	sqlite3 *connection = NULL;

	if(sqlite3_open(path.c_str(), &connection)){
		const std::string errormsg(sqlite3_errmsg(connection));
		sqlite3_close(connection);
		throw DatabaseException("Error when creating new chat database (\"" + chat.name + "\"): \"" + errormsg + "\"");
	}

	// create the messages table
	char *errormsg = NULL;
	const char *create_table =
	"create table messages (\n"
	"id integer primary key autoincrement,\n"
	"type int not null,\n" // MessageType enum in chat.h
	"unixtime int not null,\n" // unix time
	"message text not null,\n"
	"name varchar(511) not null,\n"
	"raw blob);"; // reserved for file content, image content, will be null for normal messages

	if(sqlite3_exec(connection, create_table, NULL, NULL, &errormsg)){
		const std::string err(errormsg);
		sqlite3_free(errormsg);
		throw DatabaseException("Error when creating messages table (\"" + chat.name + "\"): \"" + err + "\"");
	}

	list.emplace_back(id, chat.name, chat.creator, chat.description);
	dbs.insert({id, connection});

	save();
}

// insert a new message into database
unsigned long long Database::new_msg(const Chat &chat,const Message &msg){
	sqlite3 *const conn = get(chat.id);

	std::string insert=std::string("")+
	"insert into messages (type,unixtime,message,name,raw) values\n"
	"(?,?,?,?,?);";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn,insert.c_str(),-1,&statement,NULL);

	sqlite3_bind_int(statement,1,static_cast<int>(msg.type));
	sqlite3_bind_int(statement,2,msg.unixtime);
	sqlite3_bind_text(statement,3,msg.msg.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(statement,4,msg.sender.c_str(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_blob(statement,5,msg.raw,msg.raw_size,SQLITE_TRANSIENT);

	if(sqlite3_step(statement)!=SQLITE_DONE){
		sqlite3_finalize(statement);
		throw DatabaseException(sqlite3_errmsg(conn));
	}

	sqlite3_finalize(statement);

	// get id of last inserted item
	const std::string query=std::string("")+
	"select max(id) from messages;";
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
std::vector<Message> Database::get_messages_since(unsigned long long since, int chatid){
	sqlite3 *const conn = get(chatid);

	const std::string query=std::string("")+
	"select * from messages where id > ?;";

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

		// blob only needs to be retrieved if the Message type is IMAGE
		const MessageType type=(MessageType)sqlite3_column_int(statement, 1);
		bool getblob=type==MessageType::IMAGE;

		unsigned char *raw=NULL;
		int raw_size=0;
		if(getblob){
			// get the blob first
			raw_size=sqlite3_column_bytes(statement,5);
			const unsigned char *r=(unsigned char*)sqlite3_column_blob(statement,5);
			if(r!=NULL){
				raw=new unsigned char[raw_size];
				memcpy(raw,r,raw_size);
			}
		}

		messages.push_back({
			(decltype(Message::id))sqlite3_column_int(statement,0),
			type,
			sqlite3_column_int(statement,2),
			(char*)sqlite3_column_text(statement,3),
			(char*)sqlite3_column_text(statement,4),
			raw,
			(decltype(Message::raw_size))raw_size
		});
	}

	sqlite3_finalize(statement);

	return messages;
}

// get a file and return it
std::vector<unsigned char> Database::get_file(unsigned long long id, int chatid){
	sqlite3 *const conn = get(chatid);

	const std::string query=std::string("")+
	"select raw from messages where id=?;";

	sqlite3_stmt *statement;
	sqlite3_prepare_v2(conn, query.c_str(), -1, &statement, NULL);

	sqlite3_bind_int64(statement, 1, id);

	unsigned char *raw=NULL;
	int raw_size=0;
	int rc;
	if((rc=sqlite3_step(statement))==SQLITE_ROW){

		raw_size=sqlite3_column_bytes(statement, 0);
		const unsigned char *r=(unsigned char*)sqlite3_column_blob(statement, 0);
		if(r!=NULL){
			raw=new unsigned char[raw_size];
			memcpy(raw, r, raw_size);
		}
	}
	else{
		sqlite3_finalize(statement);
		throw DatabaseException(sqlite3_errmsg(conn));
	}

	sqlite3_finalize(statement);

	std::vector<unsigned char> buffer(raw_size);
	memcpy(&buffer[0], raw, raw_size);
	delete[] raw;

	return buffer;
}

// read the directory file, populate the <list>, and init the sqlite3 dbs
void Database::initialize(){
	bool resave = false;
	std::ifstream in(db_path + "/directory");

	while(in.good()){
		std::string entry;
		std::getline(in, entry);

		if(entry.size() != 0){
			const Chat &chat = Database::deserialize(entry);
			if(!Database::exists(db_path + "/" + std::to_string(chat.id))){
				resave = true;
				continue;
			}

			// initialize the sqlite3 connection
			sqlite3 *connection;
			if(sqlite3_open((db_path + "/" + std::to_string(chat.id)).c_str(), &connection)){
				const std::string errmsg(sqlite3_errmsg(connection));
				sqlite3_close(connection);
				throw DatabaseException("Error when initializing sqlite3 database for name:" + chat.name + ", id:" + std::to_string(chat.id) + ", reason:" + errmsg);
			}

			dbs.insert({(int)chat.id, connection});
			list.push_back(chat);
		}
	}

	if(resave)
		save();
}

sqlite3 *Database::get(int id)const{
	std::unordered_map<int, sqlite3*>::const_iterator it = dbs.find(id);

	if(it == dbs.end())
		throw DatabaseException("Could not find sqlite database for chat id " + std::to_string(id));
	return (*it).second;
}

// update the directory file
void Database::save(){
	std::string contents;

	for(const Chat &c : list)
		contents.append(Database::serialize(c) + "\n");

	std::ofstream out(db_path + "/directory");
	if(!out)
		throw DatabaseException("Error when writing to directory file");

	out << contents;
}

// return true if the file exists and does not need to be created
bool Database::exists(const std::string &file){
	return !!std::ifstream(file);
}

std::string Database::gen_name(){
	std::srand(time(NULL));

	char servername[SERVER_NAME_LENGTH + 1];
	for(int i=0;i<SERVER_NAME_LENGTH;++i){
		const char c='A'+(std::rand()%26);
		servername[i]=c;
	}

	servername[SERVER_NAME_LENGTH]=0; // terminating charactor
	return {servername};
}

// turn a chat object into a csv record
std::string Database::serialize(const Chat &chat){
	CSVWriter csv;

	char id_string[10];
	snprintf(id_string, sizeof(id_string), "%d", (int)chat.id);

	csv.add(id_string);
	csv.add(chat.name);
	csv.add(chat.creator);
	csv.add(chat.description);

	return csv.entry();
}

// get a chat object from a csv record
Chat Database::deserialize(const std::string &entry){
	CSVReader csv(entry);

	const std::optional<std::string> id_string = csv.next();
	if(!id_string)
		throw DatabaseException("no id field in entry \"" + entry + "\"");

	const std::optional<std::string> name = csv.next();
	if(!name)
		throw DatabaseException("no name field in entry \"" + entry + "\"");

	const std::optional<std::string> creator = csv.next();
	if(!creator)
		throw DatabaseException("no creator field in entry \"" + entry + "\"");

	const std::optional<std::string> description = csv.next();
	if(!description)
		throw DatabaseException("no description field in entry \"" + entry + "\"");

	int id = 0;
	if(1 != sscanf(id_string.value().c_str(), "%d", &id))
		throw DatabaseException("expected a number: " + id_string.value());
	return {(unsigned long long)id, name.value(), creator.value(), description.value()};
}

// get the chat from chat list with the highest id
int Database::highest_id(const std::vector<Chat> &chat_list){
	unsigned highest = 0;

	for(const Chat &chat : chat_list){
		if(chat.id > highest)
			highest = chat.id;
	}

	return highest;
}
