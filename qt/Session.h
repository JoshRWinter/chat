#ifndef SESSION_H
#define SESSION_H

#include <QWidget>
#include <QTextEdit>

#include <memory>

#include "Dialog.h"
#include "../client/ChatClient.h"

// custom event for cross thread communication
class Update:public QEvent{
public:
	enum class Type{
		CONNECT,
		LIST_CHATS,
		NEW_CHAT,
		SUBSCRIBE,
		MESSAGE
	};

	Update(Type t):QEvent(new_event()),eventtype(t),success(false){}
	static QEvent::Type new_event(){ return (QEvent::Type)QEvent::registerEventType(); }

	const Type eventtype;
	bool success;
	std::vector<Message> msg_list;
	std::vector<Chat> chat_list;
	Message msg;
};

class Session:public QWidget{
public:
	Session();
	void customEvent(QEvent*); // OVERRIDE from QObject

private:
	void open();
	void list_chats();
	void new_chat(const std::string&,const std::string&);
	void subscribe(const std::string&);
	void connected(const Update*);
	void listed(const Update*);
	void subscribed(const Update*);
	void new_chat_receipt(const Update*);
	void message(const Update*);
	void display_message(const Message&);
	void accept_name();
	void accept_session();

	QTextEdit *display;
	QTextEdit *inputbox;

	std::string username;
	std::string serveraddr;
	std::unique_ptr<DialogName> dname;
	std::unique_ptr<DialogSession> chooser;
	ChatClient client;
};

#endif // SESSION_H
