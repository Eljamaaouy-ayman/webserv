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
#include <ctime>

#include "../includes/server.hpp"
#include "../includes/request.hpp"
#include "../includes/RequestHandler.hpp"
#include "../includes/HttpResponse.hpp"
#include "../includes/Cgi.hpp"

const int Server::CGI_TIMEOUT_SECONDS;
const int Server::CLIENT_IDLE_TIMEOUT_SECONDS;

volatile sig_atomic_t g_shutdown = 1;

void handleSig(int)
{
	g_shutdown = 0;
}

// Constructs the server and makes a broken CGI pipe fail safely instead of raising SIGPIPE.
Server::Server()
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, handleSig);
	signal(SIGTERM, handleSig);
}

// Creates, binds, and listens on a TCP socket for the given port.
int Server::_create_server_socket(int port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		throw std::runtime_error("socket() failed");

	fcntl(fd, F_SETFL, O_NONBLOCK);
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

// Opens and registers a listening socket for every port in every server{}
// block, and remembers which block each socket belongs to so _accept_client()
// can hand new clients the right ConfigFile straight away.
void Server::init(const std::vector<ConfigFile>& configs)
{
	for (size_t i = 0; i < configs.size(); ++i)
	{
		const ConfigFile& conf = configs[i];

		for (size_t j = 0; j < conf.listen.size(); ++j)
		{
			int fd = _create_server_socket(conf.listen[j]);
			_server_fds.push_back(fd);
			_fdConf[fd] = &conf;

			pollfd pfd;
			pfd.fd = fd;
			pfd.events = POLLIN;
			pfd.revents = 0;
			_fds.push_back(pfd);
		}
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
	// server_fd tells us exactly which server{} block this client belongs
	// to -- it was recorded in init() when this listening socket was opened.
	_clients[client_fd].request->conf = *_fdConf[server_fd];

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

// How long poll() should block: -1 (forever) if no CGI is running, otherwise just
// long enough to wake up exactly when the soonest-started CGI hits its deadline.
int Server::_cgiPollTimeout()
{
	time_t now = time(NULL);
	bool found = false;
	time_t earliestDeadline = 0;

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		CgiProcess *cgi = it->second.cgi;
		if (!cgi)
			continue;

		time_t deadline = cgi->startTime + CGI_TIMEOUT_SECONDS;
		if (!found || deadline < earliestDeadline)
		{
			earliestDeadline = deadline;
			found = true;
		}
	}

	if (!found)
		return -1;

	long remaining = static_cast<long>(earliestDeadline - now);
	if (remaining < 0)
		remaining = 0;
	return static_cast<int>(remaining * 1000);
}

// Checked once per loop: kills any CGI that has been running past CGI_TIMEOUT_SECONDS,
// whether poll() woke up for it or for something unrelated (or for nothing at all).
void Server::_reapTimedOutCgi()
{
	time_t now = time(NULL);

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		CgiProcess *cgi = it->second.cgi;
		if (cgi && now - cgi->startTime >= CGI_TIMEOUT_SECONDS)
			_timeoutCgi(it->first);
	}
}

// A CGI ran past its deadline: kill it, clean up its pipes, and answer 504 instead
// of leaving the request hanging on a script that may never finish on its own.
void Server::_timeoutCgi(int clientFd)
{
	Client &client = _clients[clientFd];
	CgiProcess *cgi = client.cgi;

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

	HttpResponse response;
	response.setStatusCode(504);
	response.setErrorPage(client.request->conf);
	client.write_buff = response.build();

	delete cgi;
	client.cgi = NULL;

	struct pollfd *p = _findPollfd(clientFd);
	if (p)
		p->events = POLLIN | POLLOUT;
}

// How long poll() should block on account of idle clients: -1 (forever) if
// none are waiting, otherwise just long enough to wake up exactly when the
// soonest-idle client hits its deadline. Clients currently running a CGI are
// skipped -- they're covered by _cgiPollTimeout()/CGI_TIMEOUT_SECONDS instead.
int Server::_clientPollTimeout()
{
	time_t now = time(NULL);
	bool found = false;
	time_t earliestDeadline = 0;

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second.cgi)
			continue;

		time_t deadline = it->second.last_activity + CLIENT_IDLE_TIMEOUT_SECONDS;
		if (!found || deadline < earliestDeadline)
		{
			earliestDeadline = deadline;
			found = true;
		}
	}

	if (!found)
		return -1;

	long remaining = static_cast<long>(earliestDeadline - now);
	if (remaining < 0)
		remaining = 0;
	return static_cast<int>(remaining * 1000);
}

// Combines the CGI and idle-client deadlines into the single timeout poll() gets,
// so it never sleeps past whichever of the two is soonest.
int Server::_pollTimeout()
{
	int cgiMs = _cgiPollTimeout();
	int clientMs = _clientPollTimeout();

	if (cgiMs < 0)
		return clientMs;
	if (clientMs < 0)
		return cgiMs;
	return cgiMs < clientMs ? cgiMs : clientMs;
}

// Checked once per loop: disconnects any client (not currently running a CGI)
// that hasn't sent a single byte in CLIENT_IDLE_TIMEOUT_SECONDS, whether that's
// because they never sent a request at all or went silent after a keep-alive
// response. Collect fds first, then disconnect -- _disconnect_client() mutates
// _fds/_clients, which would invalidate the iterator above if done in-loop.
void Server::_reapTimedOutClients()
{
	time_t now = time(NULL);
	std::vector<int> timedOut;

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second.cgi)
			continue;
		if (now - it->second.last_activity >= CLIENT_IDLE_TIMEOUT_SECONDS)
			timedOut.push_back(it->first);
	}

	for (size_t k = 0; k < timedOut.size(); ++k)
	{
		for (size_t idx = 0; idx < _fds.size(); ++idx)
		{
			if (_fds[idx].fd == timedOut[k])
			{
				_disconnect_client(idx);
				break;
			}
		}
	}
}

// Runs forever: the single poll() loop driving every client socket and CGI pipe.
void Server::run()
{
	while (g_shutdown)
	{
		if (poll(_fds.data(), _fds.size(), _pollTimeout()) < 0)
			throw std::runtime_error("poll() failed");

		_reapTimedOutCgi();
		_reapTimedOutClients();

		for (size_t i = 0; i < _fds.size(); ++i)
		{
			int fd = _fds[i].fd;

			std::map<int, int>::iterator cgiIt = _cgiPipeOwner.find(fd);
			if (cgiIt != _cgiPipeOwner.end())
			{
				int clientFd = cgiIt->second;
				CgiProcess *cgi = _clients[clientFd].cgi;
				bool removed = false;

				if (cgi && fd == cgi->stdinFd && (_fds[i].revents & (POLLOUT | POLLERR | POLLHUP)))
					removed = _serviceCgiStdin(fd, clientFd);
				else if (cgi && fd == cgi->stdoutFd && (_fds[i].revents & (POLLIN | POLLHUP | POLLERR)))
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
					_clients[fd].last_activity = time(NULL);
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
							// A 400 here means the request itself was unparseable (bad request
							// line, missing/garbled Host, etc.) -- the framing is untrustworthy,
							// so we can no longer be sure where a next request would even start
							// on this connection. Close it once the response is flushed.
							if (_clients[fd].request->method == "ERROR")
								_clients[fd].shouldClose = true;
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
				{
					if (_clients[fd].shouldClose)
					{
						_disconnect_client(i);
						continue;
					}
					_fds[i].events &= ~POLLOUT;
				}
			}
		}
	}
}
