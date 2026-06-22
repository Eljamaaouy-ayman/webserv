#include "srcs/includes/request.hpp"
#include <iostream>

int main(int ac, char **av)
{
	if (ac != 2)
	{
		std::cerr << "Usage: ./webserv <config_file>\n";
		return 1;
	}
	try
	{
		ConfigFile conf;
		conf.parse_config_file(av[1]);

		if (conf.listen.empty())
			throw std::runtime_error("no listen ports defined in config");

		// --- config dump (delete this later, this is just for test) ---
		std::cout << "=== CONFIG PARSED ===\n";  
		std::cout << "host        : " << conf.host << "\n";  
		std::cout << "server_name : " << conf.server_name << "\n";  
		std::cout << "root        : " << conf.root << "\n";  
		std::cout << "index       : " << conf.index << "\n";  
		std::cout << "max_body    : " << conf.client_max_size_body << "\n";  
		std::cout << "ports       :";  
		for (size_t i = 0; i < conf.listen.size(); i++)  
			std::cout << " " << conf.listen[i];  
		std::cout << "\n";  
		for (std::map<int,std::string>::iterator it = conf.error_page.begin(); it != conf.error_page.end(); it++)  
			std::cout << "error_page  : " << it->first << " -> " << it->second << "\n";  
		for (size_t i = 0; i < conf.locations.size(); i++)  
		{  
			std::cout << "location    : " << conf.locations[i].path;  
			std::cout << "  methods:";  
			for (size_t j = 0; j < conf.locations[i].allow_methods.size(); j++)  
				std::cout << " " << conf.locations[i].allow_methods[j];  
			std::cout << "  autoindex:" << (conf.locations[i].autoindex ? "on" : "off");  
			std::cout << "\n";  
		}  
		for (std::map<std::string,std::string>::iterator it = conf.cgi_config.begin(); it != conf.cgi_config.end(); it++)  
			std::cout << "cgi         : " << it->first << " -> " << it->second << "\n";  
		std::cout << "=====================\n";  

		Server server;
		server.init(conf.listen);

		std::cout << "Server listening on port(s):";
		for (size_t i = 0; i < conf.listen.size(); i++)
			std::cout << " " << conf.listen[i];
		std::cout << "\n";

		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << "\n";
		return 1;
	}
	return 0;
}
