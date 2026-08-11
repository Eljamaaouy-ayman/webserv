#include "includes/request.hpp"

// -- previous placeholder, parsed the config but never started the server --
// int main(int ac, char ** av){
//     Request request;
//     try{
//         if (ac == 1){
//             char file[] = "config/default.conf";
//             request.conf.parse_config_file(file);
//         }
//         else if (ac == 2)
//             request.conf.parse_config_file(av[1]);
//         else
//             throw std::runtime_error("./program configFile");
//
//     } catch (const std::exception &e){
//         std::cerr << e.what() << '\n';
//         return 1;
//     }
// }

int main(int ac, char **av)
{
	if (ac > 2)
	{
		std::cerr << "Usage: ./webserv [configuration file]\n";
		return 1;
	}

	try
	{
		Request request;
		char defaultConf[] = "config/default.conf";

		if (ac == 2)
			request.conf.parse_config_file(av[1]);
		else
			request.conf.parse_config_file(defaultConf);

		if (request.conf.listen.empty())
			throw std::runtime_error("no listen ports defined in config");

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