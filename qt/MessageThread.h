#ifndef MESSAGETHREAD_H
#define MESSAGETHREAD_H

#include <QScrollArea>

#include "../chat.h"

class MessageThread;

class MessageArea:public QWidget{
public:
	MessageArea(MessageThread*);
	void add(const Message&);
	void name(const std::string&);

protected:
	void paintEvent(QPaintEvent*);

private:
	static std::string reflow(const QFontMetrics&, const std::string&, int);
	static std::vector<std::string> reflow_word(const QFontMetrics&, const std::string&, int);
	static std::vector<std::string> split(const QFontMetrics&, const std::string &text, int);
	static int line_count(const std::string&);

	MessageThread *const parent;
	std::vector<Message> msgs;
	std::string myname;
	bool scroll_to_bottom;
};

class MessageThread:public QScrollArea{
public:
	MessageThread();
	void add(const Message&);
	void name(const std::string&);
	void bottom();

private:
	MessageArea *area;
};

#endif // MESSAGETHREAD_H
