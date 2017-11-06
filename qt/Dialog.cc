#include <QBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFileDialog>

#include "Dialog.h"

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
	for(const Chat &chat:chat_list){
		auto item=new QListWidgetItem(chat.name.c_str());
		item->setToolTip(chat.description.c_str());
		list->addItem(item);
	}
	vlayout->addWidget(list);
	vlayout->addLayout(hlayout);

	QPushButton *ok = new QPushButton("Subscribe");
	QPushButton *add = new QPushButton("New Session");
	QPushButton *cancel = new QPushButton("Cancel");
	if(chat_list.size()==0)
		ok->setEnabled(false);
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
	add->setModal(true);
	add->show();
}

void DialogSession::accept_session(){
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

DialogImage::DialogImage(const QPixmap *qpm, const std::string &name):map(qpm),fname(name){
	auto save_slot=[this]{
		QFileDialog chooser(this, "Save Image");
		chooser.setAcceptMode(QFileDialog::AcceptSave);
		chooser.selectFile(fname.c_str());
		if(chooser.exec()){
			map->save(chooser.selectedFiles().at(0));
		}
	};

	setWindowTitle(fname.c_str());
	const int mw=map->rect().width()+40,mh=map->rect().height()+70;
	const int w=mw>800?800:mw,h=mh>600?600:mh;
	resize(w,h);

	auto label=new QLabel;
	auto area=new QScrollArea();
	area->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	area->setWidgetResizable(true);
	area->setWidget(label);
	auto layout=new QVBoxLayout;
	auto save=new QPushButton("Save Image");
	QObject::connect(save, &QPushButton::clicked, save_slot);

	setLayout(layout);
	label->setPixmap(*map);
	layout->addWidget(area);
	layout->addWidget(save);
}
