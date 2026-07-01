#include "srcs/includes/request.hpp"
#include <iostream>

int main(int ac, char **av)
{
	// if (ac != 2)
	// {
	// 	std::cerr << "Usage: ./webserv <config_file>\n";
	// 	return 1;
	// }
	try
	{
		// ConfigFile conf;
		Request request;
		request.conf.parse_config_file(av[1]);

		if (request.conf.listen.empty())
			throw std::runtime_error("no listen ports defined in config");

		// --- config dump (delete this later, this is just for test) ---
		// std::cout << "=== CONFIG PARSED ===\n";  
		// std::cout << "host        : " << request.conf.host << "\n";  
		// std::cout << "server_name : " << request.conf.server_name << "\n";  
		// std::cout << "root        : " << request.conf.root << "\n";  
		// std::cout << "index       : " << request.conf.index << "\n";  
		// std::cout << "max_body    : " << request.conf.client_max_size_body << "\n";  
		// std::cout << "ports       :";  
		// for (size_t i = 0; i < request.conf.listen.size(); i++)  
		// 	std::cout << " " << request.conf.listen[i];  
		// std::cout << "\n";  
		// for (std::map<int,std::string>::iterator it = request.conf.error_page.begin(); it != request.conf.error_page.end(); it++)  
		// 	std::cout << "error_page  : " << it->first << " -> " << it->second << "\n";  
		// for (size_t i = 0; i < request.conf.locations.size(); i++)  
		// {  
		// 	std::cout << "location    : " << request.conf.locations[i].path;  
		// 	std::cout << "  methods:";  
		// 	for (size_t j = 0; j < request.conf.locations[i].allow_methods.size(); j++)  
		// 		std::cout << " " << request.conf.locations[i].allow_methods[j];  
		// 	std::cout << "  autoindex:" << (request.conf.locations[i].autoindex ? "on" : "off");  
		// 	std::cout << "\n";  
		// }  
		// for (std::map<std::string,std::string>::iterator it = request.conf.cgi_config.begin(); it != request.conf.cgi_config.end(); it++)  
		// 	std::cout << "cgi         : " << it->first << " -> " << it->second << "\n";  
		// std::cout << "=====================\n";  

		Server server;
		server.init(request.conf.listen);

		std::cout << "Server listening on port(s):";
		for (size_t i = 0; i < request.conf.listen.size(); i++)
			std::cout << " " << request.conf.listen[i];
		std::cout << "\n";

		server.run(request);
		
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << "\n";
		return 1;
	}
	return 0;
}
