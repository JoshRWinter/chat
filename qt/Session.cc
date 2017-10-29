#include <QApplication>
#include <QEvent>
#include <QMessageBox>

#include <vector>

#include "Session.h"
#include "Log.h"

Session::Session():client("chatdb"){
	resize(400,600);
	setWindowTitle("ChatQT");

	// get the users name and server address
	dname.reset(new DialogName(this));
	QObject::connect(dname.get(), &QDialog::accepted, this, &Session::accept_name);
	QObject::connect(dname.get(), &QDialog::rejected, qApp, &QApplication::quit);
	dname->show();
}

// receive custom events
void Session::customEvent(QEvent *e){
	Update *event = static_cast<Update*>(e);

	switch(event->eventtype){
	case Update::Type::CONNECT:
		connected(event);
		break;
	case Update::Type::SUBSCRIBE:
		subscribed(event);
		break;
	case Update::Type::MESSAGE:
		message(event);
		break;
	}
}

// connect to server
void Session::open(){
	Update *event = new Update(Update::Type::CONNECT);
	auto callback=[this, event](bool success, std::vector<Chat> chat_list){
		event->success=success;
		event->chat_list=chat_list;

		QCoreApplication::postEvent(this, event);
	};

	client.connect(serveraddr, username, callback);
}

// subscribe a user to a chat
void Session::subscribe(const std::string &chat){
	Update *event = new Update(Update::Type::SUBSCRIBE);

	auto callback=[this, event](bool success, std::vector<Message> msg_list){
		event->success=success;
		event->msg_list=msg_list;

		QCoreApplication::postEvent(this, event);
	};

	auto msgcallback=[this, event](Message msg){

		Update *newmsg=new Update(Update::Type::MESSAGE);
		newmsg->msg=msg;

		QCoreApplication::postEvent(this, newmsg);
	};

	client.subscribe(chat, callback, msgcallback);
}

// event handler for successful connection to the server
void Session::connected(const Update *event){
	if(!event->success){
		QMessageBox box;
		box.setWindowTitle("Error");
		box.setText("Could not connect to the specified host!");
		box.exec();

		qApp->quit();
	}

	chooser.reset(new DialogSession(this, event->chat_list));
	QObject::connect(chooser.get(), &QDialog::accepted, this, &Session::accept_session);
	QObject::connect(chooser.get(), &QDialog::rejected, qApp, &QApplication::quit);
	chooser->show();
}

// event handler for subscription receipt
void Session::subscribed(const Update *event){
	if(!event->success){
		QMessageBox box;
		box.setWindowTitle("Error");
		box.setText("Could not subscribe you to the selected session for some reason (sorry)");
		box.exec();

		qApp->quit();
	}
}

// event handler for new message
void Session::message(const Update *event){
	log("message received");
}

// after user returns from DialogName
void Session::accept_name(){
	const auto [name, addr] = dname->get();
	username = name; serveraddr = addr;

	open();
}

// when the user returns from DialogSession
void Session::accept_session(){
	const std::string chat=chooser->get();

	subscribe(chat);
}
