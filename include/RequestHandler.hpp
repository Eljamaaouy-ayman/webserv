#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP
#include "HttpResponse.hpp"
#include "UserManager.hpp"
#include "HttpRequestExample.hpp"
#include "SessionManager.hpp"
#include "CreatePages.hpp"
#include "ConfigFile.hpp"
#include <sys/stat.h>
#include <limits.h>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>

#include <stdlib.h>
// #define realpath(N,R) _fullpath((R),(N),_MAX_PATH) // for windows


class HttpResponse;

class RequestHandler
{
private:
    static std::string resolvePath(const std::string &uri, const std::string &root);
    static bool isMethodAllowed(const std::string &method, location *loc);
    static void parseMultipartHeaders(const std::string &headers);
    static std::vector<std::string> parseMultipart(const std::string &body, const std::string &boundary);
    static std::string extractBoundary(const std::string &contentType);
    static std::string extractFormField(const std::string &body, const std::string &field);
    static location *getLocation(const std::string &uri);
    static HttpResponse errorResponse(int code);
    
    static HttpResponse handleLogin(const std::string &username, const std::string &password);
    static HttpResponse handleLogout(const std::string &session);
    static HttpResponse handleRegister(const std::string &username, const std::string &password);
    static HttpResponse handleMultipart(const std::string &body, const std::string &contentType);
    static HttpResponse handleAutoIndex(const std::string &path, const std::string &root);
    
    public:
    static bool isDirectory(const std::string &path);
    static std::string findContentType(const std::string &uri);
    static HttpResponse handleGET(HttpRequest &req, location *loc);
    static HttpResponse handlePOST(HttpRequest &req);
    static HttpResponse handleDELETE(HttpRequest &req, location *loc);
    static HttpResponse handleRequest(HttpRequest &req);

};
#endif