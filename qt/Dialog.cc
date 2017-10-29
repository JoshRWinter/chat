#include <QBoxLayout>
#include <QPushButton>
#include <QFormLayout>

#include "Dialog.h"
#include "Log.h"

DialogName::DialogName(QWidget *parent):QDialog(parent){
	setWindowTitle("Input your name and server address");
	resize(300, 0);

	QVBoxLayout *vlayout = new QVBoxLayout();
	QHBoxLayout *hlayout = new QHBoxLayout();
	QFormLayout *flayout = new QFormLayout();
	setLayout(vlayout);

	vlayout->addLayout(flayout);
	field_name = new QLineEdit;
	field_addr = new QLineEdit;
	flayout->addRow("Your Name", field_name);
	flayout->addRow("Server Address", field_addr);

	QPushButton *ok = new QPushButton("OK");
	QPushButton *cancel = new QPushButton("Cancel");
	QObject::connect(ok, &QPushButton::clicked, this, &QDialog::accept);
	QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
	vlayout->addLayout(hlayout);
	hlayout->addWidget(ok);
	hlayout->addWidget(cancel);
}

std::tuple<std::string, std::string> DialogName::get()const{
	return {field_name->text().toStdString(), field_addr->text().toStdString()};
}

DialogSession::DialogSession(QWidget *parent, const std::vector<Chat> &cl):QDialog(parent),chat_list(cl),newchat(false),add(NULL){
	setWindowTitle("Choose a chat Session");
	resize(350, 550);

	QVBoxLayout *vlayout = new QVBoxLayout();
	QHBoxLayout *hlayout = new QHBoxLayout();
	setLayout(vlayout);

	list = new QListWidget();
	for(const Chat &chat:chat_list)
		list->addItem(chat.name.c_str());
	vlayout->addWidget(list);
	vlayout->addLayout(hlayout);

	QPushButton *ok = new QPushButton("Subscribe");
	QPushButton *add = new QPushButton("New Session");
	QPushButton *cancel = new QPushButton("Cancel");
	QObject::connect(ok, &QPushButton::clicked, this, &QDialog::accept);
	QObject::connect(add, &QPushButton::clicked, this, &DialogSession::add_session);
	QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
	hlayout->addWidget(ok);
	hlayout->addWidget(add);
	hlayout->addWidget(cancel);
}

std::tuple<bool, std::string, std::string> DialogSession::get()const{
	const std::string &n = newchat ? name : chat_list[list->currentRow()].name;
	return {newchat, n, desc};
}

void DialogSession::add_session(){
	add = new DialogNewSession(this);
	QObject::connect(add, &QDialog::accepted, this, &DialogSession::accept_session);
	add->show();
}

void DialogSession::accept_session(){
	log("accept session");
	auto [n, d]=add->get();
	newchat=true;
	name = n; desc = d;
	accept();
}

DialogNewSession::DialogNewSession(QWidget *parent):QDialog(parent){
	setWindowTitle("New Session");

	auto vlayout = new QVBoxLayout();
	auto flayout = new QFormLayout();
	auto hlayout = new QHBoxLayout();
	setLayout(vlayout);
	vlayout->addLayout(flayout);

	flayout->addRow("Session Name", name=new QLineEdit());
	flayout->addRow("Description", desc=new QTextEdit());

	auto ok = new QPushButton("OK");
	auto cancel = new QPushButton("Cancel");
	QObject::connect(ok, &QPushButton::clicked, this, &QDialog::accept);
	QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

	vlayout->addLayout(hlayout);
	hlayout->addWidget(ok);
	hlayout->addWidget(cancel);
}

std::tuple<std::string, std::string> DialogNewSession::get()const{
	return {name->text().toStdString(), desc->toPlainText().toStdString()};
}
