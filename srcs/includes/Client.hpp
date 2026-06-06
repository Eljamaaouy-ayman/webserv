#pragma once
#include <string>

struct Client
{
	std::string read_buff;
	std::string write_buff;

	bool is_request_complete() const;
};
