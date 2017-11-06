#include <vector>
#include <iostream>

#include <QWidget>
#include <QScrollBar>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

#include "MessageThread.h"

MessageThread::MessageThread(std::function<void(const QPixmap*, const std::string&)> img_fn, std::function<void(unsigned long long, const std::string&)> file_fn){
	setSizePolicy(QSizePolicy::Policy::Expanding,QSizePolicy::Policy::Expanding);
	setWidgetResizable(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	area=new MessageArea(this, img_fn, file_fn);
	area->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	setWidget(area);
}

void MessageThread::add(const Message &m){
	area->add(m);
}

void MessageThread::bottom(){
	verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void MessageThread::name(const std::string &name){
	area->name(name);
}

MessageArea::MessageArea(MessageThread *p, std::function<void(const QPixmap*, const std::string&)> img_fn, std::function<void(unsigned long long, const std::string&)> file_fn):parent(p),img_clicked_fn(img_fn),file_clicked_fn(file_fn){}

void MessageArea::add(const Message &m){
	if(m.type==MessageType::IMAGE){
		img_cache.push_back({m.msg,m.raw,m.raw_size,m.id});
	}
	else if(m.type==MessageType::FILE){
		auto callback=[this](unsigned long long id, const std::string &filename){
			file_clicked_fn(id, filename);
		};

		btn_cache.push_back({m.id, m.msg, callback, this});
	}

	msgs.push_back(m);
	scroll_to_bottom=true;
	update();
}

void MessageArea::name(const std::string &n){
	myname=n;
}

void MessageArea::mouseReleaseEvent(QMouseEvent *event){
	const int x=event->x();
	const int y=event->y();

	for(const ImageCache &img:img_cache){
		if(x>img.x&&x<img.x+100&&y>img.y&&y<img.y+100)
			img_clicked_fn(&img.map, img.name);
	}
}

void MessageArea::paintEvent(QPaintEvent*){
	const int X_ME=45;
	const int X_THEM=5;

	QPainter painter(this);
	QSize qs=size();
	const int widget_width=qs.rwidth();
	const int boxwidth=widget_width-50;

	int y=5;
	for(const Message &msg:msgs){
		const bool me=msg.sender==myname;
		const QColor rectcolor=me?QColor(140,140,210):QColor(200,200,200);
		const QColor sendercolor=me?QColor(150,150,220):QColor(170,170,170);
		const int x=me?X_ME:X_THEM;
		const std::string formatted_name=MessageArea::reflow(painter.fontMetrics(), msg.sender, boxwidth-20);
		const int senderboxheight=painter.fontMetrics().height()*MessageArea::line_count(formatted_name)+10;

		// draw the contents of the message
		if(msg.type==MessageType::TEXT){
			const std::string formatted=MessageArea::reflow(painter.fontMetrics(), msg.msg, boxwidth-20);
			const int boxheight=painter.fontMetrics().height()*MessageArea::line_count(formatted)+20;

			painter.fillRect(x,y,boxwidth,boxheight,rectcolor);
			painter.fillRect(x,y+boxheight,boxwidth,senderboxheight,sendercolor);
			painter.drawText(QRect(x+10,y+5,boxwidth,boxheight),Qt::AlignLeft,formatted.c_str());
			painter.drawText(QRect(x+10,y+boxheight+5,boxwidth,senderboxheight),Qt::AlignLeft,formatted_name.c_str());

			y+=boxheight+senderboxheight+20;
		}
		else if(msg.type==MessageType::IMAGE){
			ImageCache *img=MessageArea::get_image(msg.id, img_cache);
			const std::string filename=MessageArea::reflow(painter.fontMetrics(), msg.msg, boxwidth-20);
			const int filenameheight=painter.fontMetrics().height()*MessageArea::line_count(filename);
			const int boxheight=120+filenameheight;

			painter.fillRect(x,y,boxwidth,boxheight,rectcolor);
			painter.fillRect(x,y+boxheight,boxwidth,senderboxheight,sendercolor);

			if(img){
				const int xpos=x+(boxwidth/2)-50;
				const int ypos=y+10;
				painter.drawPixmap(QRect(xpos,ypos,100,100), img->map, img->map.rect());
				img->x=xpos;
				img->y=ypos;
			}
			else{
				painter.drawText(x+10, y+10, "[ image error ]");
			}
			// draw the file name
			painter.drawText(QRect(x, y+115, boxwidth, filenameheight), Qt::AlignCenter, filename.c_str());

			// draw the sender's name
			painter.drawText(QRect(x+10,y+boxheight+5,boxwidth,senderboxheight),Qt::AlignLeft,formatted_name.c_str());

			y+=boxheight+senderboxheight+20;
		}
		else if(msg.type==MessageType::FILE){
			const ButtonCache *btn=MessageArea::get_btn(msg.id, btn_cache);
			const std::string filename=MessageArea::reflow(painter.fontMetrics(), msg.msg, boxwidth-20);
			const int filenameheight=painter.fontMetrics().height()*MessageArea::line_count(filename);
			const int boxheight=35+btn->button->height()+filenameheight;

			painter.fillRect(x,y,boxwidth,boxheight,rectcolor);
			painter.fillRect(x,y+boxheight,boxwidth,senderboxheight,sendercolor);

			if(btn){
				const int xpos=x+(boxwidth/2)-(btn->button->width()/2);
				const int ypos=y+10;
				btn->button->move(xpos,ypos);
			}
			else
				painter.drawText(x+10, y+10, "[ file error ]");

			// draw the file name
			painter.drawText(QRect(x, btn->button->y()+btn->button->height()+7, boxwidth, filenameheight), Qt::AlignCenter, filename.c_str());

			// draw the sender's name
			painter.drawText(QRect(x+10,y+boxheight+5,boxwidth,senderboxheight),Qt::AlignLeft,formatted_name.c_str());

			y+=boxheight+senderboxheight+20;
		}


		setMinimumHeight(y);
	}

	if(scroll_to_bottom){
		scroll_to_bottom=false;
		parent->bottom();
	}
}

std::string MessageArea::reflow(const QFontMetrics &metrics, const std::string &t, const int maxwidth){
	std::vector<std::string> words=split(metrics, t, maxwidth);

	std::vector<std::string> lines;
	std::string formatted,backup;
	unsigned wordindex=0;
	bool firstword=true;
	while(wordindex<words.size()){
		const char *const space=(wordindex==words.size()-1)?"":" ";

		backup=formatted;
		formatted+=words.at(wordindex)+space;

		if(metrics.width(formatted.c_str())>maxwidth){
			if(firstword) // something terrible happened give up
				break;

			lines.push_back(backup);
			formatted.clear();
			firstword=true;
		}
		else{
			++wordindex;
			firstword=false;
		}
	}
	lines.push_back(formatted);

	std::string finalized;
	for(const std::string &line:lines){
		const char *newline=&line==&lines[lines.size()-1]?"":"\n";
		finalized+=line+newline;
	}

	return finalized;
}

std::vector<std::string> MessageArea::reflow_word(const QFontMetrics &metrics, const std::string &word, const int maxwidth){
	std::vector<std::string> split;

	unsigned index=0;
	std::string line;
	bool firstchar=true;

	while(index<word.length()){
		line.push_back(word.at(index));

		if(metrics.width(line.c_str())>maxwidth){
			if(firstchar) // max width is too small to handle even a single character, give up
				break;
			line.pop_back();
			split.push_back(line);
			line="";
			firstchar=true;
		}
		else{
			++index;
			firstchar=false;
		}
	}
	split.push_back(line);

	return split;
}

std::vector<std::string> MessageArea::split(const QFontMetrics &metrics, const std::string &text, const int maxwidth){
	std::vector<std::string> words;
	int start=0;
	for(unsigned i=0;i<text.length();++i){
		const char next=text.length()==i+1?' ':text.at(i+1);

		if(next==' '||next=='\n'||next=='\r'){
			const std::string &sub=text.substr(start, (i+1)-start);

			if(sub.length()!=0){
				if(metrics.width(sub.c_str())>maxwidth-5){
					const std::vector<std::string> subword=reflow_word(metrics, sub, maxwidth-5);
					for(const std::string &s:subword)
						words.push_back(s);
				}
				else
					words.push_back(sub);
			}

			start=i+2;
		}
	}

	return words;
}

int MessageArea::line_count(const std::string &text){
	int count=1;

	for(char c:text){
		if(c=='\n')
			++count;
	}

	return count;
}

ImageCache *MessageArea::get_image(unsigned long long id, std::vector<ImageCache> &cache){
	for(ImageCache &item:cache)
		if(item.id==id&&item.valid)
			return &item;

	return NULL;
}

ButtonCache *MessageArea::get_btn(unsigned long long id, std::vector<ButtonCache> &cache){
	for(ButtonCache &item:cache){
		if(id==item.id){
			return &item;
		}
	}

	return NULL;
}
