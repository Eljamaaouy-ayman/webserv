#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <sys/wait.h>
#include <csignal>

#include "../includes/server.hpp"
#include "../includes/request.hpp"
#include "../includes/RequestHandler.hpp"
#include "../includes/HttpResponse.hpp"
#include "../includes/Cgi.hpp"

// Constructs the server and makes a broken CGI pipe fail safely instead of raising SIGPIPE.
Server::Server() : _conf(NULL)
{
	signal(SIGPIPE, SIG_IGN);
}

// Creates, binds, and listens on a TCP socket for the given port.
int Server::_create_server_socket(int port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		throw std::runtime_error("socket() failed");

	fcntl(fd, F_SETFD, FD_CLOEXEC);

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

// Opens and registers a listening socket for every configured port.
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

// Returns true if fd is one of the server's own listening sockets.
bool Server::_is_server_fd(int fd)
{
	return std::find(_server_fds.begin(), _server_fds.end(), fd) != _server_fds.end();
}

// Finds a pollfd entry by fd, or NULL if it isn't currently tracked.
struct pollfd *Server::_findPollfd(int fd)
{
	for (size_t i = 0; i < _fds.size(); i++)
		if (_fds[i].fd == fd)
			return &_fds[i];
	return NULL;
}

// Removes a pollfd entry by fd from _fds.
void Server::_removeFd(int fd)
{
	for (size_t i = 0; i < _fds.size(); i++)
	{
		if (_fds[i].fd == fd)
		{
			_fds.erase(_fds.begin() + i);
			return;
		}
	}
}

// Accepts a pending connection and registers it as a new poll()ed Client.
void Server::_accept_client(int server_fd)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&addr), &len);
	if (client_fd < 0)
		throw std::runtime_error("accept() failed");

	fcntl(client_fd, F_SETFL, O_NONBLOCK);
	fcntl(client_fd, F_SETFD, FD_CLOEXEC);

	_clients[client_fd] = Client();
	_clients[client_fd].request->conf = *_conf;

	pollfd pfd;
	pfd.fd = client_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_fds.push_back(pfd);
}

// Closes a client's socket (and reaps any in-flight CGI), removing it from poll().
void Server::_disconnect_client(size_t& i)
{
	int fd = _fds[i].fd;

	CgiProcess *cgi = _clients[fd].cgi;
	if (cgi)
	{
		if (cgi->stdinFd != -1)
		{
			close(cgi->stdinFd);
			_cgiPipeOwner.erase(cgi->stdinFd);
			_removeFd(cgi->stdinFd);
		}
		if (cgi->stdoutFd != -1)
		{
			close(cgi->stdoutFd);
			_cgiPipeOwner.erase(cgi->stdoutFd);
			_removeFd(cgi->stdoutFd);
		}
		kill(cgi->pid, SIGKILL);
		waitpid(cgi->pid, NULL, 0);
	}

	close(fd);
	_clients.erase(fd);

	for (i = 0; i < _fds.size(); i++)
		if (_fds[i].fd == fd)
			break;
	_fds.erase(_fds.begin() + i);
	--i;
}

// Spawns the CGI for a client's just-parsed request and registers its pipes with poll().
void Server::_startCgi(int clientFd)
{
	Client &client = _clients[clientFd];

	try
	{
		client.cgi = new CgiProcess(Cgi::start(*client.request));
	}
	catch (int code)
	{
		HttpResponse response;
		response.setStatusCode(code);
		response.setErrorPage(client.request->conf);
		client.write_buff = response.build();

		struct pollfd *p = _findPollfd(clientFd);
		if (p)
			p->events = POLLOUT;
		return;
	}

	if (client.cgi->stdinFd != -1)
	{
		pollfd pfd;
		pfd.fd = client.cgi->stdinFd;
		pfd.events = POLLOUT;
		pfd.revents = 0;
		_fds.push_back(pfd);
		_cgiPipeOwner[client.cgi->stdinFd] = clientFd;
	}

	pollfd pfd;
	pfd.fd = client.cgi->stdoutFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_fds.push_back(pfd);
	_cgiPipeOwner[client.cgi->stdoutFd] = clientFd;

	struct pollfd *clientPfd = _findPollfd(clientFd);
	if (clientPfd)
		clientPfd->events = 0;	//so poll wont report anything for it until this gets restored later. This is what stops a 2nd request on the same connection from being read and dispatched while CGI is still running
}

// Writes as much of the pending request body as the pipe currently accepts.
bool Server::_serviceCgiStdin(int pipeFd, int clientFd)
{
	CgiProcess *cgi = _clients[clientFd].cgi;

	size_t remaining = cgi->body.size() - cgi->bodyWritten;
	ssize_t n = write(pipeFd, cgi->body.c_str() + cgi->bodyWritten, remaining);

	if (n > 0)
		cgi->bodyWritten += (size_t)n;

	if (n <= 0 || cgi->bodyWritten >= cgi->body.size())
	{
		close(pipeFd);
		cgi->stdinFd = -1;
		_cgiPipeOwner.erase(pipeFd);
		_removeFd(pipeFd);
		return true;
	}
	return false;
}

// Reads available CGI output into cgi->output; on EOF/error, hands off to _finishCgi.
bool Server::_serviceCgiStdout(int pipeFd, int clientFd)
{
	CgiProcess *cgi = _clients[clientFd].cgi;

	char buf[4096];
	ssize_t n = read(pipeFd, buf, sizeof(buf));

	if (n > 0)
	{
		cgi->output.append(buf, n);
		return false;
	}

	close(pipeFd);
	cgi->stdoutFd = -1;
	_cgiPipeOwner.erase(pipeFd);
	_removeFd(pipeFd);
	_finishCgi(clientFd, n == 0);
	return true;
}

// Reaps the CGI child and turns its collected output into the client's response.
// ok=false means stdout ended in a real read() error, not clean EOF -- the
// script's output (if any) can't be trusted, so this sends a 500 instead of
// trying to parse it as a real response.
void Server::_finishCgi(int clientFd, bool ok)
{
	Client &client = _clients[clientFd];
	CgiProcess *cgi = client.cgi;

	if (cgi->stdinFd != -1)
	{
		close(cgi->stdinFd);
		_cgiPipeOwner.erase(cgi->stdinFd);
		_removeFd(cgi->stdinFd);
		cgi->stdinFd = -1;
	}

	waitpid(cgi->pid, NULL, 0);

	HttpResponse response;
	if (!ok)
	{
		response.setStatusCode(500);
		response.setErrorPage(client.request->conf);
	}
	else
	{
		try
		{
			response = Cgi::parseOutput(cgi->output);
		}
		catch (int code)
		{
			response.setStatusCode(code);
			response.setErrorPage(client.request->conf);
		}
	}
	client.write_buff = response.build();

	delete cgi;
	client.cgi = NULL;

	struct pollfd *p = _findPollfd(clientFd);
	if (p)
		p->events = POLLIN | POLLOUT;
}

// Runs forever: the single poll() loop driving every client socket and CGI pipe.
void Server::run(const Request &request)
{
	_conf = &request.conf;

	while (true)
	{
		if (poll(_fds.data(), _fds.size(), -1) < 0)
			throw std::runtime_error("poll() failed");

		for (size_t i = 0; i < _fds.size(); ++i)
		{
			int fd = _fds[i].fd;

			std::map<int, int>::iterator cgiIt = _cgiPipeOwner.find(fd);
			if (cgiIt != _cgiPipeOwner.end())
			{
				int clientFd = cgiIt->second;
				bool removed = false;

				if (_fds[i].revents & POLLOUT)
					removed = _serviceCgiStdin(fd, clientFd);
				else if (_fds[i].revents & POLLIN)
					removed = _serviceCgiStdout(fd, clientFd);

				if (removed)
					--i;
				continue;
			}

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
					_clients[fd].read_buff.append(buffer, n);

					if (_clients[fd].is_request_complete())
					{
						_clients[fd].request->setRequest(_clients[fd].read_buff);
						_clients[fd].read_buff.clear();

						if (_clients[fd].request->getIsCGI())
						{
							_startCgi(fd);
						}
						else
						{
							_clients[fd].write_buff = RequestHandler::handleRequest(*_clients[fd].request).build();
							_fds[i].events |= POLLOUT;
						}
					}
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
