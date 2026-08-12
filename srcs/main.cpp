#include "includes/request.hpp"

int main(int ac, char **av)
{
	if (ac > 2)
	{
		std::cerr << "Usage: ./webserv [configuration file]\n";
		return 1;
	}

	try
	{
		char defaultConf[] = "config/default.conf";
		char *configPath = (ac == 2) ? av[1] : defaultConf;

		std::vector<ConfigFile> configs;
		std::vector<std::string> tokens = ConfigFile::tokenize_config(configPath);
		std::vector<std::string>::iterator i = tokens.begin();

		while (i != tokens.end())
		{
			if (*i != "server")
				throw std::runtime_error("Expected server");

			ConfigFile conf;

			conf.parse_server(tokens, i);
			if (conf.listen.empty())
				throw std::runtime_error("No listen ports defined in Config");

			configs.push_back(conf);
		}

		Server server;
		server.init(configs);

		server.run();
	}

	catch(const std::exception &e)
	{
		std::cerr << e.what() << "\n";
		return (1);
	}

	return (0);
}
