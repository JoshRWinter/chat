#ifndef MESSAGETHREAD_H
#define MESSAGETHREAD_H

#include <QScrollArea>
#include <QPixmap>

#include "../chat.h"

struct ImageCache{
	ImageCache(const std::string &n, const unsigned char *raw, unsigned long long size, unsigned long long i)
	:name(n)
	,id(i)
	{
		valid=map.loadFromData(raw, size);
	}

	bool valid;
	std::string name;
	QPixmap map;
	const unsigned long long id; // message id
	int x,y; // position on screen
};

class MessageThread;

class MessageArea:public QWidget{
public:
	MessageArea(MessageThread*, std::function<void(const QPixmap*, const std::string&)>);
	void add(const Message&);
	void name(const std::string&);

protected:
	void paintEvent(QPaintEvent*);
	void mouseReleaseEvent(QMouseEvent*);

private:
	static std::string reflow(const QFontMetrics&, const std::string&, int);
	static std::vector<std::string> reflow_word(const QFontMetrics&, const std::string&, int);
	static std::vector<std::string> split(const QFontMetrics&, const std::string &text, int);
	static int line_count(const std::string&);
	static ImageCache *get_image(unsigned long long, std::vector<ImageCache>&);
	static std::string truncate(const std::string&);

	MessageThread *const parent;
	std::function<void(const QPixmap*,const std::string&)> img_clicked_fn;
	std::vector<ImageCache> img_cache;
	std::vector<Message> msgs;
	std::string myname;
	bool scroll_to_bottom;
};

class MessageThread:public QScrollArea{
public:
	MessageThread(std::function<void(const QPixmap*, const std::string&)>);
	void add(const Message&);
	void name(const std::string&);
	void bottom();

private:
	MessageArea *area;
};

#endif // MESSAGETHREAD_H
