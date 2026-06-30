#include "srcs/includes/server.hpp"
#include "srcs/includes/request.hpp"
#include <iostream>

// compile: c++ -std=c++17 faut_main.cpp srcs/server/Server.cpp srcs/server/Client.cpp -o test_server
// run:     ./test_server
//
// then in another terminal:
//   full GET  → (printf "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"; sleep 1) | nc 127.0.0.1 8080
//   full POST → (printf "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\n\r\nhello=world"; sleep 1) | nc 127.0.0.1 8080

int main()
{
	try
	{
		std::vector<int> ports;
		ports.push_back(8080);

		Server server;
		Request request;

		server.init(ports);
		std::cout << "Server listening on port 8080\n";
		server.run(request);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
}
