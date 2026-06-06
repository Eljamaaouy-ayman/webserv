#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <algorithm>

#include "../includes/server.hpp"

int Server::_create_server_socket(int port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		throw std::runtime_error("socket() failed");

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
		throw std::runtime_error("bind() failed");

	if (listen(fd, SOMAXCONN) < 0)
		throw std::runtime_error("listen() failed");

	return fd;
}

void Server::init(const std::vector<int>& ports)
{
	for (size_t i = 0; i < ports.size(); ++i)
	{
		int fd = _create_server_socket(ports[i]);
		_server_fds.push_back(fd);

		pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		_fds.push_back(pfd);
	}
}

bool Server::_is_server_fd(int fd)
{
	return std::find(_server_fds.begin(), _server_fds.end(), fd) != _server_fds.end();
}

void Server::_accept_client(int server_fd)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&addr), &len);
	if (client_fd < 0)
		throw std::runtime_error("accept() failed");

	fcntl(client_fd, F_SETFL, O_NONBLOCK);

	_clients[client_fd] = Client();

	pollfd pfd;
	pfd.fd = client_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_fds.push_back(pfd);
}

void Server::_disconnect_client(size_t& i)
{
	int fd = _fds[i].fd;
	close(fd);
	_clients.erase(fd);
	_fds.erase(_fds.begin() + i);
	--i;
}

void Server::run()
{
	while (true)
	{
		if (poll(_fds.data(), _fds.size(), -1) < 0)
			throw std::runtime_error("poll() failed");

		for (size_t i = 0; i < _fds.size(); ++i)
		{
			int fd = _fds[i].fd;

			if (_fds[i].revents & POLLIN)
			{
				if (_is_server_fd(fd))
				{
					_accept_client(fd);
					continue;
				}
				char buffer[1025];
				ssize_t n = recv(fd, buffer, 1024, 0);
				if (n > 0)
				{
					buffer[n] = '\0';
					_clients[fd].read_buff += buffer;
					_clients[fd].write_buff = "Echo: " + _clients[fd].read_buff;
					_fds[i].events |= POLLOUT;
				}
				else
				{
					_disconnect_client(i);
					continue;
				}
			}
			if (_fds[i].revents & POLLOUT)
			{
				std::string& buf = _clients[fd].write_buff;
				ssize_t n = send(fd, buf.c_str(), buf.size(), 0);
				if (n > 0)
					buf.erase(0, n);
				if (buf.empty())
					_fds[i].events &= ~POLLOUT;
			}
		}
	}
}
