#include <iostream>
#include "ftpClient.h"


using namespace std;

int main(int argc, char* argv[])
{
	ftpClient client;
	stringstream cmd;
	string responce;

	if(argc != 2)
	{
		cerr << "Musí být zadaný parametr" << endl;
		return -1;
	}

	try
	{

		client.parseInput(argv[1]);
		client.establishConnection(false);
		client.getResponce(false);

		cmd << "USER " << client.connection.username << endl;
		client << cmd;
		client.getResponce(false);

		cmd << "PASS " << client.connection.password << endl;
		client << cmd;
		client.getResponce(false);

		client.passive();

		cmd << "CWD " << client.connection.path << endl;
		client << cmd;
		client.getResponce(false);

		cmd << "NLST" << endl;
		client << cmd;
		client.getResponce(false);
		client.getResponce(true);
		client.getResponce(false);

		cmd << "QUIT" << endl;
		client << cmd;
	}
	catch(ftpException & e)
	{
		e.printMessage();
		return e.getCode();
	}

	return 0;
}


