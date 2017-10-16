#ifndef CHATWORKUNIT_H
#define CHATWORKUNIT_H

#include "network.h"

struct Command{
	virtual ~Command(){}
	virtual void process(net::tcp&)const=0;
};

#endif // CHATWORKUNIT_H
