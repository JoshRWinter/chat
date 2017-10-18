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

	client.connect("localhost","Josh Winter",[&client](bool success, std::vector<Chat> list){
		if(success){
			if(list.size()==0)
				print("no chats");
			for(const Chat &chat:list){
				print(std::string("id: ")+std::to_string(chat.id)+" name: "+chat.name+" creator: "+chat.creator+" description: "+chat.description);
			}

			auto subscribe_callback=[](bool success,std::vector<Message> list){
				print(std::string("here are the (")+std::to_string(list.size())+") since last connect:");
				for(const Message &msg:list){
					print(std::to_string(msg.id)+" "+msg.msg);
				}
			};

			auto msg_callback=[](Message msg){
				print(std::string("received msg ")+std::to_string(msg.id)+" "+msg.msg);
			};

			client.subscribe(list[0].id,list[0].name,subscribe_callback,msg_callback);
		}
		else
			print("couldn't connect");
	});

	/*
	client.newchat("ayyy there","sweet",[](bool success){
		print(success?"success!":"failure!");
	});
	*/

	std::string input;
	do{
		print("enter msg: ");
		std::getline(std::cin,input);
		client.send(input);
	}while(input!="quit");

	/*
	while(running.load()){
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	*/

	return 0;
};
