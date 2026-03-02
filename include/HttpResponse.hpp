#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "RequestHandler.hpp"
#include "ConfigFile.hpp"


class HttpResponse
{
private:
    int statusCode;
    std::map<std::string, std::string> headers;
    std::string body_;
    std::vector<std::string> cookies;
    std::string getReasonPhrase();

public:
    void setStatusCode(int code);
    void addHeader(const std::string &key, const std::string &value);
    void addCookie(const std::string &key, const std::string &value, const std::string &path);
    void addCookie(const std::string &key, const std::string &value, const std::string &path, int maxAge);
    void setBody(const std::string &body);
    void setErrorPage();
    std::string build();
    
};

#endif