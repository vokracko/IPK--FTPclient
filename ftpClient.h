#include <string>
#include <cstring>
#include <sstream>
#include <stack>
#include <regex.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include "ftpException.h"

struct Tconnection
{
	std::string username;
	std::string password;
	std::string host;
	std::string path;
	unsigned int port;
};

enum connectionIndexes
{
	HOST = 2,
	PORT = 4,
	PATH = 5,
};


class ftpClient
{
		static const int replyLength = 99;

		std::stack<std::string *> responces;
		int controlSocket;
		int passiveSocket;

		// static enum class indexes
		// {
		// 	HOST = 2,
		// 	PORT = 4,
		// 	PATH = 5,
		// };

	public:
		Tconnection connection;

		ftpClient();
		~ftpClient();

		void establishConnection(bool passive);
		void parseInput(const char * input);
		void passiveAddress();
		void pushResponce(const char * message);
		void operator << (std::stringstream & command);

		bool isLast();
		void send(std::stringstream & command);
		bool passive();

		int crlf(const char * message);
		int commas(char * str, int n);

		std::string * getResponce(int exceptedCode, bool passive);
		std::string & getUsername();
};
