#pragma once
#include <string>
#include <ctime>

class Request;
struct CgiProcess;

struct Client
{
	std::string read_buff;
	std::string write_buff;
	Request *request;
	CgiProcess *cgi;
	time_t last_activity; //updated on accept and on every successful recv(); used to time out idle connections
	bool shouldClose; //set once a 400 (malformed/unparseable request) is queued; disconnect once write_buff is fully flushed

	Client();
	Client(const Client &other);
	Client &operator=(const Client &other);
	~Client();

	bool is_request_complete();

private:
	bool _dechunkIfComplete(size_t header_end);
};
