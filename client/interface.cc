#include <iostream>
#include <mutex>
#include <atomic>

#include <signal.h>

#include "ChatClient.cc"

static std::atomic<bool> running;

void handler(int sig){
	switch(sig){
	case SIGPIPE:
		break;
	case SIGTERM:
	case SIGINT:
		running.store(false);
	}
}

static std::mutex out_lock;
void print(const std::string &line){
	std::lock_guard<std::mutex> lock(out_lock);
	std::cout<<line<<std::endl;
}

int main(){
	signal(SIGTERM,handler);
	signal(SIGINT,handler);
	signal(SIGPIPE,handler);
	running.store(true);
	ChatClient client("chatdb");

	client.connect("localhost","Josh Winter",[](bool success, std::vector<Chat> list){
		if(success){
			if(list.size()==0)
				print("no chats");
			for(const Chat &chat:list){
				print(std::string("id: ")+std::to_string(chat.id)+" name: "+chat.name+" creator: "+chat.creator+" description: "+chat.description);
			}
		}
		else
			print("couldn't connect");
	});

	Chat chat("coolc\"\"hat","joe biden","this is a sick chat");
	client.newchat(chat,[](bool success){
		print(success?"success!":"failure!");
	});

	while(running.load()){
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	return 0;
};
