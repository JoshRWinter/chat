#include <QApplication>

#include "Session.h"

int main(int argc, char **argv){
	QApplication app(argc,argv);

	Session session;
	session.show();

	return app.exec();
}
