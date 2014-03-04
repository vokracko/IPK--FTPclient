#include <iostream>
#include "ftpClient.h"

//TODO regulár  user + password
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
		client.getResponce(220, false);

		cmd << "USER " << client.connection.username << endl;
		client << cmd;
		client.getResponce(331, false);

		cmd << "PASS " << client.connection.password << endl;
		client << cmd;
		client.getResponce(230, false);

		client.passive();

		cmd << "CWD " << client.connection.path << endl;
		client << cmd;
		client.getResponce(250, false);

		cmd << "NLST" << endl;
		client << cmd;

		client.getResponce(150, false);
		client.getResponce(0, true);
		client.getResponce(226, false);

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


