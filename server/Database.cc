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
			throw std::runtime_error("Error, server name not 25 characters");
	}

	// create the directory file
	if(!Database::exists(directory_path)){
		// create the directory file
		std::ofstream dirfile(directory_path);
	}

	// populate the chat list and dbs map
	initialize();
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
	lite3::connection conn(path);

	// create the messages table
	const std::string create_table =
	"create table messages (\n"
	"id integer primary key autoincrement,\n"
	"type int not null,\n" // MessageType enum in chat.h
	"unixtime int not null,\n" // unix time
	"message text not null,\n"
	"name varchar(511) not null,\n"
	"raw blob);"; // reserved for file content, image content, will be null for normal messages

	conn.execute(create_table);

	list.emplace_back(id, chat.name, chat.creator, chat.description);
	dbs.emplace(id, std::move(conn));

	save();
}

// insert a new message into database
unsigned long long Database::new_msg(const Chat &chat,const Message &msg){
	lite3::connection &conn = get(chat.id);

	const std::string insert =
	"insert into messages (type,unixtime,message,name,raw) values\n"
	"(?,?,?,?,?);";

	lite3::statement statement(conn, insert);

	statement.bind(1, static_cast<int>(msg.type));
	statement.bind(2, msg.unixtime);
	statement.bind(3, msg.msg);
	statement.bind(4, msg.sender);
	statement.bind(5, msg.raw, msg.raw_size);

	statement.execute();

	// get id of last inserted item
	const std::string query =
	"select max(id) from messages;";
	lite3::statement maxid(conn, query);

	maxid.execute();
	return maxid.integer(0);
}

// get all messages from chat <name> where id is bigger than <since>
std::vector<Message> Database::get_messages_since(unsigned long long since, int chatid){
	lite3::connection &conn = get(chatid);

	const std::string query =
	"select * from messages where id > ?;";
	lite3::statement statement(conn, query);

	statement.bind(1, (std::int64_t)since);

	std::vector<Message> messages;
	while(statement.execute()){
		// blob only needs to be retrieved if the Message type is IMAGE
		const MessageType type = (MessageType)statement.integer(1);

		unsigned char *raw=NULL;
		int raw_size=0;
		if(type == MessageType::IMAGE){
			// get the blob first
			raw_size = statement.blob_size(5);
			const unsigned char *const r = (unsigned char*)statement.blob(5);
			if(r != NULL){
				// must copy it from sqlite's memory
				raw=new unsigned char[raw_size];
				memcpy(raw, r, raw_size);
			}
		}

		messages.push_back({
			(decltype(Message::id))statement.integer(0),
			type,
			statement.integer(2),
			statement.str(3),
			statement.str(4),
			raw,
			(decltype(Message::raw_size))raw_size
		});
	}

	return messages;
}

// get a file and return it
std::vector<unsigned char> Database::get_file(unsigned long long id, int chatid){
	lite3::connection &conn = get(chatid);

	const std::string query =
	"select raw from messages where id=?;";
	lite3::statement statement(conn, query);

	statement.bind(1, (std::int64_t)id);

	std::vector<unsigned char> raw;
	if(statement.execute()){
		raw.resize(statement.blob_size(0));
		const unsigned char *const r = (unsigned char*)statement.blob(0);

		if(r != NULL)
			memcpy(raw.data(), r, raw.size());
		else
			throw std::runtime_error("raw blob for file id " + std::to_string(id) + ", chatid " + std::to_string(chatid));
	}
	else
		throw std::runtime_error("no record for message id " + std::to_string(id) + ", chatid " + std::to_string(chatid));

	return raw;
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
			lite3::connection connection(db_path + "/" + std::to_string(chat.id));

			dbs.emplace(chat.id, std::move(connection));
			list.push_back(chat);
		}
	}

	if(resave)
		save();
}

lite3::connection &Database::get(int id){
	std::unordered_map<int, lite3::connection>::iterator it = dbs.find(id);

	if(it == dbs.end())
		throw std::runtime_error("Could not find sqlite database for chat id " + std::to_string(id));
	return (*it).second;
}

// update the directory file
void Database::save(){
	std::string contents;

	for(const Chat &c : list)
		contents.append(Database::serialize(c) + "\n");

	std::ofstream out(db_path + "/directory");
	if(!out)
		throw std::runtime_error("Error when writing to directory file");

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
		const char c='!'+(std::rand()%94);
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
		throw std::runtime_error("no id field in entry \"" + entry + "\"");

	const std::optional<std::string> name = csv.next();
	if(!name)
		throw std::runtime_error("no name field in entry \"" + entry + "\"");

	const std::optional<std::string> creator = csv.next();
	if(!creator)
		throw std::runtime_error("no creator field in entry \"" + entry + "\"");

	const std::optional<std::string> description = csv.next();
	if(!description)
		throw std::runtime_error("no description field in entry \"" + entry + "\"");

	int id = 0;
	if(1 != sscanf(id_string.value().c_str(), "%d", &id))
		throw std::runtime_error("expected a number: " + id_string.value());
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
