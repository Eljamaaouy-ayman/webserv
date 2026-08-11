#include "../includes/Client.hpp"
#include "../includes/request.hpp"
#include "../includes/Cgi.hpp"
#include <cstdlib>

Client::Client() : request(new Request()), cgi(NULL)
{
}

Client::Client(const Client &other)
	: read_buff(other.read_buff), write_buff(other.write_buff),
	  request(new Request(*other.request)), cgi(NULL)
{
}


Client &Client::operator=(const Client &other)
{
	if (this != &other)
	{
		read_buff = other.read_buff;
		write_buff = other.write_buff;
		delete request;
		request = new Request(*other.request);
		delete cgi;
		cgi = NULL;
	}
	return *this;
}

Client::~Client()
{
	delete request;
	delete cgi;
}

// Returns true once read_buff holds a full HTTP request (headers + body).
bool Client::is_request_complete()
{
	size_t header_end = read_buff.find("\r\n\r\n");
	if (header_end == std::string::npos)
		return false;

	size_t te_pos = read_buff.find("Transfer-Encoding: chunked");
	if (te_pos != std::string::npos && te_pos < header_end)
		return _dechunkIfComplete(header_end);

	size_t cl_pos = read_buff.find("Content-Length:");
	if (cl_pos != std::string::npos && cl_pos < header_end)
	{
		size_t val_start = cl_pos + 15;
		while (val_start < read_buff.size() && read_buff[val_start] == ' ')
			val_start++;
		size_t val_end = read_buff.find("\r\n", val_start);
		size_t content_length = (size_t)std::atoi(read_buff.substr(val_start, val_end - val_start).c_str());
		return (read_buff.size() - (header_end + 4)) >= content_length;
	}
	return true;
}

// Decodes a chunked body in place once fully received; false if still incomplete.
bool Client::_dechunkIfComplete(size_t header_end)
{
	size_t pos = header_end + 4;
	std::string decoded;

	while (true)
	{
		size_t line_end = read_buff.find("\r\n", pos);
		if (line_end == std::string::npos)
			return false;

		std::string sizeToken = read_buff.substr(pos, line_end - pos);
		size_t semicolon = sizeToken.find(';');
		if (semicolon != std::string::npos)
			sizeToken = sizeToken.substr(0, semicolon);

		size_t chunkSize = (size_t)std::strtoul(sizeToken.c_str(), NULL, 16);
		size_t dataStart = line_end + 2;

		if (chunkSize == 0)
		{
			if (read_buff.find("\r\n", dataStart) == std::string::npos)
				return false;
			break;
		}

		if (read_buff.size() < dataStart + chunkSize + 2)
			return false;

		decoded.append(read_buff, dataStart, chunkSize);
		pos = dataStart + chunkSize + 2;
	}

	std::string headerBlock = read_buff.substr(0, header_end);
	std::string rebuilt;
	size_t lineStart = 0;
	while (lineStart <= headerBlock.size())
	{
		size_t lineEnd = headerBlock.find("\r\n", lineStart);
		std::string line = (lineEnd == std::string::npos)
								? headerBlock.substr(lineStart)
								: headerBlock.substr(lineStart, lineEnd - lineStart);
		if (line.find("Transfer-Encoding:") != 0)
			rebuilt += line + "\r\n";
		if (lineEnd == std::string::npos)
			break;
		lineStart = lineEnd + 2;
	}

	std::ostringstream lenStream;
	lenStream << decoded.size();
	rebuilt += "Content-Length: " + lenStream.str() + "\r\n";

	read_buff = rebuilt + "\r\n" + decoded;
	return true;
}

//example of chenked 

// POST /upload HTTP/1.1\r\n
// Host: localhost:8080\r\n
// Transfer-Encoding: chunked\r\n
// \r\n
// 5\r\n
// Hello\r\n
// 6\r\n
//  World\r\n
// 0\r\n
// \r\n