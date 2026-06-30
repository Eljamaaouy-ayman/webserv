#pragma once

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.hpp"
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

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

  private:
    std::map<std::string, std::string> request;
    std::map<std::string, std::string> session;
	std::string	_cgiResponse;

  public:
    std::string method;
    std::string path;
    std::string httpV;
    bool isCGI;
    ConfigFile conf;

    // * CGI information
    struct CgiInfo {
      CgiInfo();
  
      std::string host;
      std::string port;
      std::string method;
      std::string scriptPath;
      std::string pathInfo;
      std::string query;
      std::map<std::string,std::string> headers;
      std::string body;
      size_t contentLength;
      std::string contentType;
    };

    // * obj of cgi struct
    CgiInfo cgi;


    // * Default Contructor
    Request();

    void setRequest(const std::string &req);
    const std::map<std::string, std::string> &getRequest() const;

    void setSession(const std::string session_id, const std::string value);
    const std::map<std::string, std::string> &getSession() const;
    
    void checkCGI(std::string path);
    const bool &getIsCGI() const;

    bool pathGCIisFile(std::string path);
	void	setCgiResponse(const std::string& cgiResponse);
	const std::string& getCgiResponse() const;
	std::string			getMethodByName(std::string methodName);
};


// class Request{
//     public:
//         std::string path;
//         ConfigFile conf;
// };