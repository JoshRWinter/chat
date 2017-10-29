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

DialogSession::DialogSession(QWidget *parent, const std::vector<Chat> &cl):QDialog(parent),chat_list(cl){
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
	QPushButton *cancel = new QPushButton("Cancel");
	QObject::connect(ok, &QPushButton::clicked, this, &QDialog::accept);
	QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
	hlayout->addWidget(ok);
	hlayout->addWidget(cancel);
};

std::string DialogSession::get()const{
	return chat_list[list->currentRow()].name;
}
