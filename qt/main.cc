#include <fstream>

#include <QApplication>

#include "Session.h"

static std::string getdbpath();
static std::string getprofilepath();
static std::tuple<std::string, std::string> loadconfig();
static void saveconfig(const std::string&, const std::string&);

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int){
	int argc=0;
	char **argv=NULL;
#else
int main(int argc, char **argv){
#endif // _WIN32
	const auto [name, addr]=loadconfig();

	QApplication app(argc,argv);

	Session session(name, addr, getdbpath());
	session.show();

	auto rc=app.exec();

	const auto [newname, newaddr]=session.get_names();
	if(newname.length()==0&&newaddr.length()==0)
		saveconfig(name, addr);
	else
		saveconfig(newname, newaddr);
	return rc;
}

std::tuple<std::string, std::string> loadconfig(){
	const std::string &path=getprofilepath();

	std::ifstream in(path);
	if(!in)
		return {"", ""};

	std::string name, addr;
	std::getline(in, name);
	std::getline(in, addr);

	return {name, addr};
}

void saveconfig(const std::string &name, const std::string &addr){
	std::ofstream out(getprofilepath());
	if(!out)
		return;

	out<<name<<'\n';
	out<<addr<<'\n';
}

#ifdef _WIN32
#include <windows.h>
std::string getdbpath(){
	const char *path="%userprofile%\\chat.db";
	char expanded[150]="INVALID PATH";

	ExpandEnvironmentStrings(path, expanded, 149);
	return expanded;
}
std::string getprofilepath(){
	const char *path="%userprofile%\\chat.profile.txt";
	char expanded[150]="INVALID PATH";

	ExpandEnvironmentStrings(path, expanded, 149);
	return expanded;
}
#else
#include <wordexp.h>
std::string getdbpath(){
	wordexp_t p;

	wordexp("~/.chatdb", &p, 0);
	const std::string fqpath=p.we_wordv[0];
	wordfree(&p);

	return fqpath;
}
std::string getprofilepath(){
	wordexp_t p;

	wordexp("~/.chatprofile", &p, 0);
	const std::string fullpath=p.we_wordv[0];
	wordfree(&p);

	return fullpath;
}
#endif // _WIN32
