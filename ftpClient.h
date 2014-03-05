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

class ftpClient
{
		static const int replyLength = 99;

		std::stack<std::string *> responces;
		int controlSocket;
		int passiveSocket;

		enum
		{
			USERNAME = 3,
			PASSWORD = 4,
			HOST = 5,
			PORT = 7,
			PATH = 8,
		};

	public:
		Tconnection connection;

		ftpClient();
		~ftpClient();

		void establishConnection(bool passive);
		void parseInput(const char * input);
		void passiveAddress();
		void percentEncoding(std::string & str);
		void pushResponce(const char * message);
		void operator << (std::stringstream & command);

		bool isLast();
		void send(std::stringstream & command);
		bool passive();

		int crlf(const char * message);
		int commas(char * str, int n);

		std::string * getResponce(int exceptedCode1, int exceptedCode2 = 0, bool passive = false);
		std::string * lastResponce();
		std::string & getUsername();
};
