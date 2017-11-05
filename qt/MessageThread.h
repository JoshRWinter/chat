#ifndef MESSAGETHREAD_H
#define MESSAGETHREAD_H

#include <QScrollArea>
#include <QPixmap>
#include <QPushButton>

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

struct ButtonCache{
	ButtonCache(const long long unsigned i, const std::string &name, std::function<void(unsigned long long, const std::string&)> fn, QWidget *parent):id(i),filename(name){
		auto onclick=[i, name, fn]{
			fn(i, name);
		};

		button=new QPushButton("Download File", parent);
		QObject::connect(button, &QPushButton::clicked, onclick);
		button->show();
	}

	const long long unsigned id;
	const std::string filename;
	QPushButton *button;
};

class MessageThread;

class MessageArea:public QWidget{
public:
	MessageArea(MessageThread*, std::function<void(const QPixmap*, const std::string&)>, std::function<void(long long unsigned, const std::string&)>);
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
	static ButtonCache *get_btn(unsigned long long, std::vector<ButtonCache>&);

	MessageThread *const parent;
	std::function<void(const QPixmap*,const std::string&)> img_clicked_fn;
	std::function<void(long long unsigned,const std::string&)> file_clicked_fn;
	std::vector<ImageCache> img_cache;
	std::vector<ButtonCache> btn_cache;
	std::vector<Message> msgs;
	std::string myname;
	bool scroll_to_bottom;
};

class MessageThread:public QScrollArea{
public:
	MessageThread(std::function<void(const QPixmap*, const std::string&)>, std::function<void(unsigned long long,const std::string&)>);
	void add(const Message&);
	void name(const std::string&);
	void bottom();

private:
	MessageArea *area;
};

#endif // MESSAGETHREAD_H
