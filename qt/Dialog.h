#include <QDialog>
#include <QLineEdit>
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

class DialogSession:public QDialog{
public:
	DialogSession(QWidget*, const std::vector<Chat>&);
	std::string get()const;

private:
	QListWidget *list;
	const std::vector<Chat> chat_list;
};
