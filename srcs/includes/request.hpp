#pragma once

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.hpp"

typedef struct location {
    std::string path;
    std::vector<std::string> allow_methods;
    bool autoindex;
    std::string root;
    std::string return_to;
    std::string index;
} location;


class ConfigFile {
    public:
        std::vector<int> listen;
        std::string server_name;
        std::string host;
        std::string root;
        int client_max_size_body;
        std::string index;
        std::map<int, std::string> error_page;
        std::vector<location> locations;
        std::map<std::string, std::string>cgi_config;

        void parse_config_file(char *av);
};

class Request{
    public:
        std::string path;
        ConfigFile conf;
};