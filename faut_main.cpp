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
		std::vector<ConfigFile> configs;
		std::vector<std::string> tokens = ConfigFile::tokenize_config(av[1]);
		std::vector<std::string>::iterator i = tokens.begin();

		while (i != tokens.end())
		{
			if (*i != "server")
				throw std::runtime_error("expected server");

			ConfigFile conf;

			conf.parse_server(tokens, i);
            if (conf.listen.empty())
			throw std::runtime_error("no listen ports defined in config");

			configs.push_back(conf);
		}

        for (size_t i = 0; i < configs.size(); ++i)
        {
            ConfigFile& c = configs[i];

            std::cout << "\n========== SERVER " << i + 1 << " ==========\n";

            // Basic server information
            std::cout << "server_name: " << c.server_name << std::endl;
            std::cout << "host: " << c.host << std::endl;
            std::cout << "root: " << c.root << std::endl;
            std::cout << "index: " << c.index << std::endl;
            std::cout << "client_max_body_size: "
                    << c.client_max_size_body << std::endl;

            // Listen ports
            std::cout << "\nlisten:\n";
            for (size_t j = 0; j < c.listen.size(); ++j)
            {
                std::cout << "  - " << c.listen[j] << std::endl;
            }

            // Error pages
            std::cout << "\nerror pages:\n";
            for (std::map<int, std::string>::iterator it = c.error_page.begin();
                it != c.error_page.end();
                ++it)
            {
                std::cout << "  " << it->first
                        << " -> " << it->second << std::endl;
            }

            // CGI configuration
            std::cout << "\nCGI config:\n";
            for (std::map<std::string, std::string>::iterator it =
                    c.cgi_config.begin();
                it != c.cgi_config.end();
                ++it)
            {
                std::cout << "  " << it->first
                        << " -> " << it->second << std::endl;
            }

            // Locations
            std::cout << "\nLocations:\n";

            for (size_t j = 0; j < c.locations.size(); ++j)
            {
                location& loc = c.locations[j];

                std::cout << "  ---------- LOCATION "
                        << j + 1 << " ----------\n";

                std::cout << "  path: " << loc.path << std::endl;
                std::cout << "  root: " << loc.root << std::endl;
                std::cout << "  index: " << loc.index << std::endl;
                std::cout << "  return: " << loc.return_to << std::endl;
                std::cout << "  autoindex: "
                        << (loc.autoindex ? "on" : "off")
                        << std::endl;

                std::cout << "  allow_methods:";
                for (size_t k = 0; k < loc.allow_methods.size(); ++k)
                {
                    std::cout << " " << loc.allow_methods[k];
                }
                std::cout << std::endl;
            }

            std::cout << "====================================\n";
        }
		Server server;
		server.init(configs);

		std::cout << "Server listening on port(s):";
		for (size_t i = 0; i < configs.size(); ++i)
			for (size_t j = 0; j < configs[i].listen.size(); ++j)
				std::cout << " " << configs[i].listen[j];
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
