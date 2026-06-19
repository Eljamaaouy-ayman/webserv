#ifndef HTTPREQUESTEXAMPLE_HTTP
#define HTTPREQUESTEXAMPLE_HTTP

#include <string>
#include <map>

struct HttpRequest {
    std::string method;
    std::string uri;
    std::string body;
    std::string contentType;
    std::map<std::string, std::string> headers;
};

#endif` 