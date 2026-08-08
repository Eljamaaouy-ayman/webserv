#pragma once
#include <string>

class Request;
struct CgiProcess;

struct Client
{
	std::string read_buff;
	std::string write_buff;
	Request *request;
	CgiProcess *cgi;

	Client();
	Client(const Client &other);
	Client &operator=(const Client &other);
	~Client();

	bool is_request_complete();

private:
	bool _dechunkIfComplete(size_t header_end);
};
