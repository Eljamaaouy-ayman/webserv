#pragma once

#include <vector>
#include <map>
#include <poll.h>
#include "../server/Client.hpp"

class Server {
public:
    void init(const std::vector<int>& ports);
    void run();

private:
    std::vector<pollfd>   _fds;
    std::map<int, Client> _clients;
    std::vector<int>      _server_fds;

    int  _create_server_socket(int port);
    void _accept_client(int server_fd);
    void _disconnect_client(size_t& i);
    bool _is_server_fd(int fd);
};
