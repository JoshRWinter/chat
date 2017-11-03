#include <vector>
#include <iostream>

#include <QWidget>
#include <QScrollBar>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

#include "MessageThread.h"

MessageThread::MessageThread(){
	setSizePolicy(QSizePolicy::Policy::Expanding,QSizePolicy::Policy::Expanding);
	setWidgetResizable(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	area=new MessageArea(this);
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

MessageArea::MessageArea(MessageThread *p):parent(p){}

void MessageArea::add(const Message &m){
	if(m.type==MessageType::IMAGE){
		img_cache.push_back({m.raw,m.raw_size,m.id});
	}

	msgs.push_back(m);
	scroll_to_bottom=true;
	update();
}

void MessageArea::name(const std::string &n){
	myname=n;
}

void MessageArea::paintEvent(QPaintEvent*){
	const int X_ME=45;
	const int X_THEM=5;

	QPainter painter(this);
	QSize qs=size();
	const int widget_width=qs.rwidth();
	const int widget_height=qs.rheight();
	const int boxwidth=widget_width-50;

	int y=5;
	for(const Message &msg:msgs){
		const bool me=msg.sender==myname;
		const QColor rectcolor=me?QColor(120,120,190):QColor(200,200,200);
		const QColor sendercolor=me?QColor(150,150,220):QColor(170,170,170);
		const int x=me?X_ME:X_THEM;
		const std::string formatted_name=MessageArea::reflow(painter.fontMetrics(), msg.sender, boxwidth-10);
		const int senderboxheight=painter.fontMetrics().height()*MessageArea::line_count(formatted_name)+10;

		// draw the contents of the message
		if(msg.type==MessageType::TEXT){
			const std::string formatted=MessageArea::reflow(painter.fontMetrics(), msg.msg, boxwidth-10);
			const int boxheight=painter.fontMetrics().height()*MessageArea::line_count(formatted)+20;

			painter.fillRect(x,y,boxwidth,boxheight,rectcolor);
			painter.fillRect(x,y+boxheight,boxwidth,senderboxheight,sendercolor);
			painter.drawText(QRect(x+5,y+5,boxwidth,boxheight),Qt::AlignLeft,formatted.c_str());
			painter.drawText(QRect(x+5,y+boxheight+5,boxwidth,senderboxheight),Qt::AlignLeft,formatted_name.c_str());

			y+=boxheight+senderboxheight+20;
		}
		else if(msg.type==MessageType::IMAGE){
			const QPixmap *map=MessageArea::get_image(msg.id, img_cache);
			const std::string filename=MessageArea::reflow(painter.fontMetrics(), msg.msg, boxwidth-10);
			const int filenameheight=painter.fontMetrics().height()*MessageArea::line_count(filename);
			const int boxheight=70+filenameheight;

			painter.fillRect(x,y,boxwidth,boxheight,rectcolor);
			painter.fillRect(x,y+boxheight,boxwidth,senderboxheight,sendercolor);

			if(map){
				painter.drawPixmap(QRect(x+(boxwidth/2)-25,y+10,50,50), *map, map->rect());
			}
			else{
				painter.drawText(x+10, y+10, "[ image error ]");
			}
			// draw the file name
			painter.drawText(QRect(x+5, y+65, boxwidth, filenameheight), Qt::AlignLeft, filename.c_str());

			painter.drawText(QRect(x+5,y+boxheight+5,boxwidth,senderboxheight),Qt::AlignLeft,formatted_name.c_str());

			y+=boxheight+senderboxheight+20;
		}
		else if(msg.type==MessageType::FILE){
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
	int wordindex=0;
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

	int index=0;
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
	for(int i=0;i<text.length();++i){
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

const QPixmap *MessageArea::get_image(unsigned long long id, const std::vector<ImageCache> &cache){
	for(const ImageCache &item:cache)
		if(item.id==id&&item.valid)
			return &item.map;

	return NULL;
}
