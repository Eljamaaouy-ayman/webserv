#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include <vector>
#include <string>
#include <map>

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
        static std::vector<int> listen;
        static std::string server_name;
        static std::string host;
        static std::string root;
        static int client_max_size_body;
        static std::string index;
        static std::map<int, std::string> error_page;
        static std::vector<location> locations;
        static std::map<std::string, std::string>cgi_config;

        // static void parse_config_file(char *av);
};

#endif