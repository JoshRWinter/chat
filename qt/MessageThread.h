#ifndef MESSAGETHREAD_H
#define MESSAGETHREAD_H

#include <QScrollArea>
#include <QPixmap>

#include "../chat.h"

struct ImageCache{
	ImageCache(const unsigned char *raw, unsigned long long size, unsigned long long i)
	:id(i)
	{
		valid=map.loadFromData(raw, size);
	}

	bool valid;
	QPixmap map;
	const unsigned long long id; // message id
};

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
	static const QPixmap *get_image(unsigned long long, const std::vector<ImageCache>&);

	MessageThread *const parent;
	std::vector<ImageCache> img_cache;
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
