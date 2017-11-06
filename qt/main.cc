#include <QApplication>

#include "Session.h"

static std::string getdbpath();

int main(int argc, char **argv){
	QApplication app(argc,argv);

	Session session(getdbpath());
	session.show();

	return app.exec();
}

#include <wordexp.h>
std::string getdbpath(){
	wordexp_t p;

	wordexp("~/.chatdb", &p, 0);
	std::string fqpath=p.we_wordv[0];
	wordfree(&p);

	return fqpath;
}
