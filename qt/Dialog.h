#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>

#include <tuple>
#include <vector>

#include "../client/ChatClient.h"

class DialogName:public QDialog{
public:
	DialogName(QWidget*);
	std::tuple<std::string, std::string> get()const;

private:
	QLineEdit *field_name, *field_addr;
};

class DialogNewSession:public QDialog{
public:
	DialogNewSession(QWidget*);
	std::tuple<std::string, std::string> get()const;

private:
	QLineEdit *name;
	QTextEdit *desc;
};

class DialogSession:public QDialog{
public:
	DialogSession(QWidget*, const std::vector<Chat>&);
	std::tuple<bool, std::string, std::string> get()const;

private:
	void add_session();
	void accept_session();

	bool newchat;
	DialogNewSession *add;
	std::string name,desc;
	QListWidget *list;
	const std::vector<Chat> chat_list;
};

class DialogImage:public QDialog{
public:
	DialogImage(const QPixmap*, const std::string&);

private:
	const QPixmap *const map;
	const std::string fname;
};

#endif // DIALOG_H
