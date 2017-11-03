#include <QApplication>
#include <QEvent>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>

#include <vector>
#include <iostream>

#include "Session.h"

Session::Session():client("chatdb"){
	resize(400,600);
	setWindowTitle("ChatQT");

	auto action = [this](){
		client.send(inputbox->toPlainText().toStdString());
		inputbox->setText("");
	};
	display=new MessageThread;
	inputbox=new TextBox(action);
	inputbox->setMaximumHeight(100);
	auto sender=new QPushButton("Send");
	QObject::connect(sender, &QPushButton::clicked, action);

	auto vlayout=new QVBoxLayout;
	setLayout(vlayout);
	vlayout->addWidget(display);
	vlayout->addWidget(inputbox);
	vlayout->addWidget(sender);

	// get the users name and server address
	dname.reset(new DialogName(this));
	QObject::connect(dname.get(), &QDialog::accepted, this, &Session::accept_name);
	QObject::connect(dname.get(), &QDialog::rejected, qApp, &QApplication::quit);
	dname->setModal(true);
	dname->show();

	inputbox->setFocus();
}

// receive custom events
void Session::customEvent(QEvent *e){
	Update *event = dynamic_cast<Update*>(e);

	switch(event->eventtype){
	case Update::Type::CONNECT:
		connected(event);
		break;
	case Update::Type::LIST_CHATS:
		listed(event);
		break;
	case Update::Type::NEW_CHAT:
		new_chat_receipt(event);
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
	auto callback=[this, event](bool success){
		event->success=success;

		QCoreApplication::postEvent(this, event);
	};

	client.connect(serveraddr, username, callback);
}

// refresh the chat list from the server
void Session::list_chats(){
	Update *event = new Update(Update::Type::LIST_CHATS);
	auto callback=[this, event](std::vector<Chat> chat_list){
		event->chat_list=chat_list;

		QCoreApplication::postEvent(this, event);
	};

	client.list_chats(callback);
}

// allow the user to make a new chat
void Session::new_chat(const std::string &name, const std::string &description){
	auto event = new Update(Update::Type::NEW_CHAT);

	auto callback=[this, event](bool success){
		event->success = success;

		QCoreApplication::postEvent(this, event);
	};

	client.newchat(name, description, callback);
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

	list_chats();
}

// event handler for chat list from server
void Session::listed(const Update *event){
	chooser.reset(new DialogSession(this, event->chat_list));
	QObject::connect(chooser.get(), &QDialog::accepted, this, &Session::accept_session);
	QObject::connect(chooser.get(), &QDialog::rejected, qApp, &QApplication::quit);
	chooser->setModal(true);
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

	for(const Message &msg:event->msg_list)
		display_message(msg);
}

// event handler for new chat receipt
void Session::new_chat_receipt(const Update *event){
	const char *msg = event->success?
	"New chat session successfully created":
	"Could not create new chat session";

	QMessageBox box(this);
	box.setWindowTitle("Info");
	box.setText(msg);
	box.exec();

	list_chats();
}

// event handler for new message
void Session::message(const Update *event){
	display_message(event->msg);
}

void Session::display_message(const Message &msg){
	display->add(msg);
}

// after user returns from DialogName
void Session::accept_name(){
	const auto [name, addr] = dname->get();
	username=name;
	serveraddr=addr;

	open();
	username=client.name();
	display->name(username);
}

// when the user returns from DialogSession
void Session::accept_session(){
	const auto [newchat, name, description] = chooser->get();

	if(newchat){
		// user wants to make a new chat
		new_chat(name, description);
	}
	else{
		subscribe(name);
	}
}
