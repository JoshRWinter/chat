#ifndef SESSION_H
#define SESSION_H

#include <QWidget>
#include <QTextEdit>
#include <QKeyEvent>

#include <memory>

#include "Dialog.h"
#include "MessageThread.h"
#include "../client/ChatClient.h"

// custom event for cross thread communication
class Update:public QEvent{
public:
	enum class Type{
		CONNECT,
		LIST_CHATS,
		NEW_CHAT,
		SUBSCRIBE,
		MESSAGE,
		MESSAGE_RECEIPT,
		GET_FILE
	};

	Update(Type t):QEvent(new_event()),eventtype(t),success(false){}
	static QEvent::Type new_event(){ return (QEvent::Type)QEvent::registerEventType(); }

	const Type eventtype;
	bool success;
	std::vector<Message> msg_list;
	std::vector<Chat> chat_list;
	unsigned long long raw_size;
	Message msg;
	unsigned char *raw;
	std::string filename;
	std::string errmsg;
	std::string name;
};

class TextBox:public QTextEdit{
public:
	TextBox(std::function<void()> fn):enter(fn){}
	void keyPressEvent(QKeyEvent *event){
		if(event->key()==Qt::Key_Return){
			enter();
		}
		else
			QTextEdit::keyPressEvent(event);
	}

private:
	std::function<void()> enter;
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
	void get_file(unsigned long long, const std::string &);
	void connected(const Update*);
	void listed(const Update*);
	void subscribed(const Update*);
	void new_chat_receipt(const Update*);
	void message(const Update*);
	void receipt_received(const Update*);
	void file_received(const Update*);
	void display_message(const Message&);
	void accept_name();
	void accept_session();
	void slotImage();
	void slotFile();
	std::function<void(bool,const std::string&)> receipt;
	static unsigned char *read_file(const std::string&,int&);
	static bool write_file(const std::string&,unsigned char*, int);
	static std::string truncate(const std::string&);

	MessageThread *display;
	QTextEdit *inputbox;

	std::string username;
	std::string serveraddr;
	std::unique_ptr<DialogName> dname;
	std::unique_ptr<DialogSession> chooser;
	ChatClient client;
};

#endif // SESSION_H
