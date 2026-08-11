#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP
#include "HttpResponse.hpp"
#include "UserManager.hpp"
#include "request.hpp"
#include "SessionManager.hpp"
#include "CreatePages.hpp"
#include "includes.hpp"
#include <sys/stat.h>
#include <limits.h>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>


#include <stdlib.h>

class HttpResponse;

typedef struct Part
{
    std::map<std::string, std::string> headers;
    std::string filename;
    std::string name;
    std::string body;
} Part;

class RequestHandler
{
private:
    static bool isMethodAllowed(const std::string &method, location *loc);
    static std::string urlDecode(std::string &str);
    static std::string getFieldValue(const std::string &body, const std::string &field);
    static location *getLocation(const std::string &uri, std::vector<location> &locations);
    static HttpResponse errorResponse(int code, ConfigFile &conf);
    static bool isAuthenticated(Request &req);
    static void parseContentDisposition(Part &part);
    static std::vector<Part> parseMultipart(const std::string &body, const std::string &boundary);
    static std::string extractBoundary(const std::string &contentType);

    static HttpResponse handleLogin(const std::string &username, const std::string &password);
    static HttpResponse handleLogout(const std::string &session);
    static HttpResponse handleRegister(const std::string &username, const std::string &password);
    static HttpResponse handleUpload(Request &req, location *loc);
    static HttpResponse handleAutoIndex(const std::string &path, const std::string &root);

public:
    static bool isDirectory(const std::string &path);
    static std::string findContentType(const std::string &uri);
    static HttpResponse handleGET(Request &req, location *loc);
    static HttpResponse handlePOST(Request &req, location *loc);
    static HttpResponse handleDELETE(Request &req, location *loc);
    static HttpResponse handleRequest(Request &req);
};


#endif