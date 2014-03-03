#include "ftpClient.h"

ftpClient::ftpClient()
{
	connection.username = "anonymous";
	connection.password = "anonymous@host.com";
	connection.path = "./";
}

ftpClient::~ftpClient()
{
	while(responces.size())
	{
		delete responces.top();
		responces.pop();
	}

	close(passiveSocket);
	close(controlSocket);
}


void ftpClient::parseInput(const char * input)
{
	bool compRes;
	bool match;
	regex_t regex;
	regmatch_t parts[PATH+1]; //PATH je poslední položka ve struktuře

	//TODO do regexu doplnit username:password@

	compRes = regcomp(&regex, "^(ftp://)?([a-zA-Z0-9.-]+)(:([0-9]+))?(/[a-zA-Z0-9_\\.-]+)*[/]?$", REG_EXTENDED);

	if(compRes != 0)
	{
		throw ftpException(ftpException::REGEX_COMPILATION);
	}

	match = regexec(&regex, input, PATH+1, parts, 0);

	if(match != 0)
	{
		regfree(&regex);
		throw ftpException(ftpException::REGEX_MATCH);
	}

	connection.host.assign(input, parts[HOST].rm_so, parts[HOST].rm_eo - parts[HOST].rm_so);

	if(parts[PORT].rm_so != -1)
	{
		connection.port = atoi(input + parts[PORT].rm_so);
	}
	else
	{
		connection.port = 21;
	}

	if(parts[PATH].rm_so != -1)
	{
		connection.path.assign(input, parts[PATH].rm_so, parts[PATH].rm_eo - parts[PATH].rm_so);
	}

	regfree(&regex);
}


void ftpClient::establishConnection(bool passive)
{
	sockaddr_in address;
	hostent *host = gethostbyname(connection.host.c_str());
	int sck = socket(PF_INET, SOCK_STREAM, 0);
	int res;

	if(sck < 0) throw ftpException(ftpException::SOCKET);
	if(host == NULL) throw ftpException(ftpException::HOSTNAME);

	address.sin_family = PF_INET;
	address.sin_port = htons(connection.port);
	std::memcpy(&address.sin_addr, host->h_addr, host->h_length);

	res = connect(sck, (struct sockaddr *) &address, sizeof(address));

	if(res < 0) throw ftpException(ftpException::CONNECT);

	if(passive)
	{
		passiveSocket = sck;
	}
	else
	{
		controlSocket = sck;
	}
}

std::string * ftpClient::getResponce(int exceptedCode, bool passive)
{
	//TODO konec zprávy: kód následuje mezerou
	static char reply[replyLength+1] = {0};
	int socket = passive ? passiveSocket : controlSocket;
	int res;

	do
	{
		res = recv(socket, reply, replyLength, 0);

		if(passive) std::cout << reply;

		pushResponce(reply);
		memset(reply, 0, replyLength);
	}
	while(!isLast() && res > 0);

	if(!passive && atoi(responces.top()->c_str()) != exceptedCode)
	{
		throw ftpException(ftpException::RESPONCE);
		std::cerr << responces.top()->c_str() << std::endl;
	}

	std::cout << responces.top()->c_str() << std::endl;

	return responces.top();
}

void ftpClient::pushResponce(const char * message)
{
	static bool messageEnded = true;
	std::string * tmp;
	int crlfPos;

	if(!messageEnded)
	{
		crlfPos = crlf(message);

		if(crlfPos == -1)
		{
			responces.top()->append(message);
			return;
		}
		else
		{
			responces.top()->append(message, crlfPos);
			message += crlfPos + 1;
			messageEnded = true;
		}
	}

	while((crlfPos = crlf(message)) != -1)
	{
		tmp = new std::string(message, crlfPos);
		responces.push(tmp);
		messageEnded = true;
		message += crlfPos + 1;
		// std::cout << "start: " << start << std::endl;
	}

	//více zpráv na řádku, poslední neukončená
	if(strlen(message) > 0)
	{
		tmp = new std::string(message);
		responces.push(tmp);
		messageEnded = false;
	}

}

// bool ftpClient::startWithCode(const char * message)
// {
// 	return isdigit(reply[0]) && isdigit(reply[1]) && isdigit(reply[2]);
// }

int ftpClient::crlf(const char * message)
{
	int index = 0;
	int length = strlen(message);

	while(index < length && message[index] != '\r' && message[index+1] != '\n') ++index;

	return index == length ? -1 : index+1;
}

bool ftpClient::isLast()
{
	const char * top = responces.top()->c_str();

	return top[3] == ' ' && (top[strlen(top)-1] == '\n' || top[strlen(top)-1] == '\r');
}

void ftpClient::operator << (std::stringstream & command)
{
	ftpClient::send(command);
}

void ftpClient::send(std::stringstream & command)
{
	//TODO ověřovat res?
	int res;
	res = ::send(controlSocket, command.str().c_str(), command.str().size(), 0);
	command.str("");
}

bool ftpClient::passive()
{
	std::stringstream cmd("PASV\r\n");
	ftpClient::send(cmd);
	getResponce(227, false);
	passiveAddress();
	establishConnection(true);

	return true;
}


void ftpClient::passiveAddress()
{
	std::string * message = responces.top();
	char * start = const_cast<char*>(message->c_str()) + message->length();
	int port;
	int n;

	while(*start-- != '(');
	start += 2;
	n = commas(start, 4);

	connection.host.assign(start, n);
	start += n + 1;
	connection.port = atoi(start) * 256;
	n = commas(start, 1);
	start += n + 1;
	connection.port += atoi(start);
}

int ftpClient::commas(char * str, int n)
{
	int index = 0;
	int occurence = 0;

	while(str[index] != '\0')
	{
		if(str[index] == ',')
		{
			occurence++;
			str[index] = '.';
		}

		if(occurence == n) return index;
		index++;
	}

	throw ftpException(ftpException::NOT_FOUND);
}
