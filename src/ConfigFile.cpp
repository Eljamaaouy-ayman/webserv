#include <vector>
#include <string>
#include <map>
#include "../include/ConfigFile.hpp"

std::vector<int> ConfigFile::listen;
std::string ConfigFile::server_name;
std::string ConfigFile::host;
std::string ConfigFile::root;
int ConfigFile::client_max_size_body;
std::string ConfigFile::index;
std::map<int, std::string> ConfigFile::error_page;
// std::vector<location> locations;
std::map<std::string, std::string> ConfigFile::cgi_config;

// void parse_config_file(char *av);