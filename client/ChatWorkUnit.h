#ifndef CHATWORKUNIT_H
#define CHATWORKUNIT_H

#include <functional>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "../chat.h"

enum class WorkUnitType:std::uint8_t{
	CONNECT, // connect to a server
	LIST_CHATS, // refresh the chat list
	NEW_CHAT, // have the server make a new chat
	SUBSCRIBE, // subscribe to a chat
	MESSAGE, // send a message
	GET_FILE // requesting a file from the server
};

struct ChatWorkUnit{
	ChatWorkUnit(WorkUnitType t):type(t){}
	virtual ~ChatWorkUnit(){};
	const WorkUnitType type;
};

// for connecting to the server
struct ChatWorkUnitConnect:ChatWorkUnit{
	ChatWorkUnitConnect(const std::string &t,const std::string &n,std::function<void(bool,const std::string&)> fn)
	:ChatWorkUnit(WorkUnitType::CONNECT)
	,target(t)
	,myname(n)
	,callback(fn)
	{}

	const std::string target;
	const std::string myname;
	const std::function<void(bool,const std::string&)> callback;
};

struct ChatWorkUnitListChats:ChatWorkUnit{
	ChatWorkUnitListChats(std::function<void(std::vector<Chat>)> fn)
	:ChatWorkUnit(WorkUnitType::LIST_CHATS)
	,callback(fn)
	{}

	const std::function<void(std::vector<Chat>)> callback;
};

// for creating a new chat
struct ChatWorkUnitNewChat:ChatWorkUnit{
	ChatWorkUnitNewChat(const std::string &n,const std::string &d,std::function<void(bool)> fn)
	:ChatWorkUnit(WorkUnitType::NEW_CHAT)
	,name(n)
	,desc(d)
	,callback(fn)
	{}

	const std::string name;
	const std::string desc;
	const std::function<void(bool)> callback;
};

// for subscribing to a chat
struct ChatWorkUnitSubscribe:ChatWorkUnit{
	ChatWorkUnitSubscribe(const std::string &n,std::function<void(bool,std::vector<Message>)> c,std::function<void(Message)> m)
	:ChatWorkUnit(WorkUnitType::SUBSCRIBE)
	,name(n)
	,callback(c)
	,msg_callback(m)
	{}

	const std::string name;
	const std::function<void(bool,std::vector<Message>)> callback;
	const std::function<void(Message)> msg_callback;
};

// for sending a message
struct ChatWorkUnitMessage:ChatWorkUnit{
	ChatWorkUnitMessage(MessageType t,const std::string &m,unsigned char *r,unsigned long long rs, std::atomic<int> *pcnt, std::function<void(bool,const std::string&)> fn)
	:ChatWorkUnit(WorkUnitType::MESSAGE)
	,type(t)
	,text(m)
	,raw(r)
	,raw_size(rs)
	,percent(pcnt)
	,callback(fn)
	{}

	const MessageType type;
	const std::string text;
	unsigned char *const raw;
	const unsigned long long raw_size;
	std::atomic<int> *const percent;
	std::function<void(bool,const std::string&)> callback;
};

// for getting a file
struct ChatWorkUnitGetFile:ChatWorkUnit{
	ChatWorkUnitGetFile(unsigned long long i, std::atomic<int> *pcnt, std::function<void(const unsigned char*,int)> fn)
	:ChatWorkUnit(WorkUnitType::GET_FILE)
	,id(i)
	,percent(pcnt)
	,callback(fn)
	{}

	const unsigned long long id;
	std::atomic<int> *const percent;
	std::function<void(const unsigned char*,int)> callback;
};

class ChatWorkQueue{
public:
	~ChatWorkQueue(){
		std::lock_guard<std::mutex> lock(mutex);
		while(work.size() > 0){
			const ChatWorkUnit *const unit = work.front();
			work.pop();
			delete unit;
		}
	}

	void push(const ChatWorkUnit *unit){
		{
			std::lock_guard<std::mutex> lock(mutex);
			work.push(unit);
		}

		cvar.notify_one();
	}

	const ChatWorkUnit *pop_wait(int millis){
		auto predicate = [this]{
			return work.size() != 0;
		};

		std::chrono::duration<int, std::ratio<1, 1000>> timeout(millis);

		std::unique_lock<std::mutex> lock(mutex);
		if(cvar.wait_for(lock, timeout, predicate)){
			const ChatWorkUnit *unit = work.front();
			work.pop();
			return unit;
		}
		else
			return NULL;
	}

	int count(){
		std::lock_guard<std::mutex> lock(mutex);
		return work.size();
	}

private:
	std::condition_variable cvar;
	std::mutex mutex;
	std::queue<const ChatWorkUnit*> work;
};

#endif // CHATWORKUNIT_H
