#include "ftpClient.h"

ftpClient::ftpClient()
{
	connection.username = "anonymous";
	connection.password = "anonymous";
	connection.path = "./";
	connection.port = 21;
}

ftpClient::~ftpClient()
{
	while(responces.size())
	{
		delete responces.top();
		responces.pop();
	}

	close(controlSocket);
}


void ftpClient::parseInput(const char * input)
{
	bool compRes;
	bool match;
	regex_t regex;
	regmatch_t parts[PATH+1]; //PATH je poslední položka ve struktuře

	compRes = regcomp(&regex, "^(ftp://(([^:@]+):([^@]+)@)?)?([a-zA-Z0-9.-]+)(:([0-9]+))?(/.*)?$", REG_EXTENDED);

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

	if(parts[USERNAME].rm_so != -1)
	{
		connection.username.assign(input, parts[USERNAME].rm_so, parts[USERNAME].rm_eo - parts[USERNAME].rm_so);
		percentEncoding(connection.username);
	}

	if(parts[PASSWORD].rm_so != -1)
	{
		connection.password.assign(input, parts[PASSWORD].rm_so, parts[PASSWORD].rm_eo - parts[PASSWORD].rm_so);
		percentEncoding(connection.password);
	}

	connection.host.assign(input, parts[HOST].rm_so, parts[HOST].rm_eo - parts[HOST].rm_so);

	if(parts[PORT].rm_so != -1)
	{
		connection.port = atoi(input + parts[PORT].rm_so);
	}

	if(parts[PATH].rm_so != -1)
	{
		connection.path.assign(input, parts[PATH].rm_so, parts[PATH].rm_eo - parts[PATH].rm_so);
	}

	regfree(&regex);
}

void ftpClient::percentEncoding(std::string & str)
{
	const int matchSize = str.length() / 3;
	bool compRes;
	bool match;
	char buff[3] = {0, 0, '\0'};
	regex_t regex;
	regmatch_t parts[matchSize];

	compRes = regcomp(&regex, "(\\%[0-9A-F]{1}[0-9A-F]{1})", REG_EXTENDED);

	if(compRes != 0)
	{
		throw ftpException(ftpException::REGEX_COMPILATION);
	}

	while(regexec(&regex, str.c_str(), matchSize, parts, 0) == 0)
	{
		memcpy(buff, str.c_str() + parts[1].rm_so + 1, 2);
		int number = strtol(buff, NULL, 16);
		str.replace(parts[1].rm_so, 3, (char*) &number, 0, 1);
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

std::string * ftpClient::getResponce(int exceptedCode1, int exceptedCode2, bool passive)
{
	static char reply[replyLength+1] = {0};
	int socket = passive ? passiveSocket : controlSocket;
	int res;

	do
	{
		res = recv(socket, reply, replyLength, 0);

		if(passive) std::cout << reply;
		else pushResponce(reply);
		memset(reply, 0, replyLength);
	}
	while((!passive && !isLast()) || (passive && res != 0));

	int code = atoi(responces.top()->c_str());
	if(passive) close(passiveSocket);
	if(!passive && (exceptedCode1 != code && exceptedCode2 != code))
	{
		throw ftpException(ftpException::RESPONCE);
		std::cerr << responces.top()->c_str() << std::endl;
	}

	return responces.top();
}

void ftpClient::pushResponce(char * message)
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
	}

	//více zpráv na řádku, poslední neukončená
	if(strlen(message) > 0)
	{
		tmp = new std::string(message);
		responces.push(tmp);
		messageEnded = false;
	}

}

std::string * ftpClient::lastResponce()
{
	return responces.size() ? responces.top() : NULL;
}

int ftpClient::crlf(char * message)
{
	char * index = strstr(message, "\r\n");

	return index ? (int) (index - message) + 1: -1;
}

bool ftpClient::isLast()
{
	const char * top = responces.top()->c_str();
	return isdigit(top[0]) && isdigit(top[1]) && isdigit(top[2]) && top[3] == ' ' && (top[strlen(top)-1] == '\n' || top[strlen(top)-1] == '\r');
}

void ftpClient::operator << (std::stringstream & command)
{
	ftpClient::send(command);
}

void ftpClient::send(std::stringstream & command)
{
	::send(controlSocket, command.str().c_str(), command.str().size(), 0);
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
