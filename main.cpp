#include <iostream>
#include "ftpClient.h"

//TODO občas se zasekne
using namespace std;

int main(int argc, char* argv[])
{
	ftpClient client;
	stringstream cmd;
	string responce;

	if(argc != 2)
	{
		cerr << "Musí být zadán právě jeden parametr" << endl;
		return -1;
	}

	try
	{
		client.parseInput(argv[1]);
		client.establishConnection(false);
		client.getResponce(220);

		cmd << "USER " << client.connection.username << "\r\n";
		client << cmd;
		client.getResponce(331, 230);

		cmd << "PASS " << client.connection.password << "\r\n";
		client << cmd;
		client.getResponce(230);

		client.passive();

		cmd << "CWD " << client.connection.path << "\r\n";
		client << cmd;
		client.getResponce(250);

		cmd << "LIST" << "\r\n";
		client << cmd;

		client.getResponce(150);
		client.getResponce(0, 0, true);
		client.getResponce(226);

		cmd << "QUIT" << endl;
		client << cmd;
	}
	catch(ftpException & e)
	{
		if(client.lastResponce()) cerr << client.lastResponce()->c_str() << endl;
		cerr << e.getMessage() << endl;
		return e.getCode();
	}

	return 0;
}


