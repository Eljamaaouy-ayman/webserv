#include "srcs/includes/server.hpp"
#include <iostream>

int main()
{
	try
	{
		std::vector<int> ports;
		ports.push_back(8080);

		Server server;
		server.init(ports);
		std::cout << "Server listening on port 8080\n";
		server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
}
