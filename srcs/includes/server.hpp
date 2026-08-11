#pragma once

#include <vector>
#include <map>
#include <poll.h>
#include "Client.hpp"

class Request;
class ConfigFile;
struct CgiProcess;

class Server {
public:
    Server();

    void init(const std::vector<int>& ports);
    void run(const Request &request);

private:
    static const int CGI_TIMEOUT_SECONDS = 5; //a CGI running longer than this gets killed and answered with 504

    std::vector<pollfd>   _fds; //listen sockets + client sockets + cgi pipes
    std::map<int, Client> _clients;
    std::vector<int>      _server_fds;
    const ConfigFile      *_conf;
    std::map<int, int>    _cgiPipeOwner; //cgi pipe fd -> which client it belongs to

    int  _create_server_socket(int port);
    void _accept_client(int server_fd);
    void _disconnect_client(size_t& i);
    bool _is_server_fd(int fd);

    struct pollfd *_findPollfd(int fd);
    void _removeFd(int fd);

    void _startCgi(int clientFd);
    bool _serviceCgiStdin(int pipeFd, int clientFd);
    bool _serviceCgiStdout(int pipeFd, int clientFd);
    void _finishCgi(int clientFd, bool ok = true);

    int  _cgiPollTimeout();
    void _reapTimedOutCgi();
    void _timeoutCgi(int clientFd);
};
