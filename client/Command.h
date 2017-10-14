#ifndef COMMAND_H
#define COMMAND_H

#include "network.h"

struct Command{
	virtual ~Command(){}
	virtual void process(net::tcp&)const=0;
};

#endif // COMMAND_H
