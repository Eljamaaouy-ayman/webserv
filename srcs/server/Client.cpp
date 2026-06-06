#include "../includes/Client.hpp"
#include <cstdlib>

bool Client::is_request_complete() const
{
	size_t header_end = read_buff.find("\r\n\r\n");
	if (header_end == std::string::npos)
		return false;

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
