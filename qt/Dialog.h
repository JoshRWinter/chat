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
	DialogName(const std::string&, const std::string&, QWidget*);
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

	std::string name,desc;
	QListWidget *list;
	const std::vector<Chat> chat_list;
	bool newchat;
	DialogNewSession *add;
};

class DialogImage:public QDialog{
public:
	DialogImage(QWidget*, const QPixmap*, const std::string&);

private:
	const QPixmap *const map;
	const std::string fname;
};

class DialogProgress:public QDialog{
public:
	DialogProgress(QWidget*, const std::string &, const std::atomic<int>&, bool);
};

#endif // DIALOG_H
