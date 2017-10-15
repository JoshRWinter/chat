#include <chrono>

#include "ChatService.h"
#include "log.h"

ChatService::ChatService():
	working(true),
	command_count(0),
	handle(std::ref(*this))
{
	log("started service thread");
}

ChatService::~ChatService(){
	working.store(false);
	handle.join();
	log("joined service thread");
}

// accessed from multiple threads
void ChatService::add_command(Command *cmd){
	std::lock_guard<std::mutex> lock(mutex);

	commands.push(std::unique_ptr<Command>(cmd));
	++command_count;
}

// entry point for the worker thread
void ChatService::operator()(){
	try{
		loop();
	}catch(const NetworkException &e){
		reconnect();
	}catch(const ShutdownException &e){
		log_error(std::string("shutting down with ")+std::to_string(command_count.load())+" commands left in the queue!");
	}
}

// command loop
void ChatService::loop(){
	while(working.load()){

		// process commands
		if(command_count.load()>0){
			std::unique_ptr<Command> command=get_command();

			try{
				command->process(tcp);
			}catch(const NetworkException &e){
				log_error(e.what());
				// add the command back to the queue to retry again later
				add_command(command.release());
				throw;
			}
		}
	}
}

// try to reconnect
void ChatService::reconnect(){
	log_error("lost connection! attempting to reconnect");

	bool reconnected=false;
	while(!reconnected){
		reconnected=tcp.connect();

		// see if services needs to shut down
		if(!working.load())
			throw ShutdownException();

		// don't burn out the cpu
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	log("reconnected successfully");
}

// get the first command in the list, then erase it from the list, the return it
std::unique_ptr<Command> &&ChatService::get_command(){
	std::lock_guard<std::mutex> lock(mutex);

	Command *cmd=commands.front().release();
	commands.pop();
	--command_count;

	return std::move(std::unique_ptr<Command>(cmd));
}

void ChatService::send(void *data,int size){
	int sent=0;
	while(sent!=size){
		sent+=tcp.send_nonblock((char*)data+sent,size-sent);

		if(!tcp)
			throw NetworkException();

		if(!working.load())
			throw ShutdownException();
	}
}

void ChatService::recv(void *data,int size){
	int got=0;
	while(got!=size){
		got+=tcp.recv_nonblock((char*)data+got,size-got);

		if(!tcp)
			throw NetworkException();

		if(!working.load())
			throw ShutdownException();
	}
}
