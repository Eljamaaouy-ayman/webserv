#pragma once

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.hpp"
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <algorithm>

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

        static std::vector<std::string> tokenize_config(char *av);

        void parse_server(
            std::vector<std::string>& tokens,
            std::vector<std::string>::iterator& i
        );

        void parse_location(
            std::vector<std::string>& tokens,
            std::vector<std::string>::iterator& i
        );
};


class Request{

  private:
    
    bool parseRequestLine(const std::string& line);
    bool parseHeaders(const std::string& req);
    void parseBody(const std::string& req);
    void validateAndStoreRequest();
    void processMultipartBody();
    void setupCgiInfo();
    void clearRequestData();

  public:
  std::map<std::string, std::string> request;
  std::map<std::string, std::string> session;
	std::string	_cgiResponse;
    std::string method;
    std::string path;
    std::string httpV;
    bool isCGI;
    // The one server{} block that applies to this connection, assigned by
    // Server::_accept_client() based on which listening port was connected to.
    ConfigFile conf;
    bool _foundCookie;

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

    void setSession(const std::string value);
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