#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP
#include "HttpResponse.hpp"
#include "UserManager.hpp"
#include "SessionManager.hpp"
#include "CreatePages.hpp"
#include "ConfigFile.hpp"

class HttpResponse;

class RequestHandler
{
private:
    static void parseMultipartHeaders(const std::string &headers);
    static std::vector<std::string> parseMultipart(const std::string &body, const std::string &boundary);
    static std::string extractBoundary(const std::string &contentType);
    static std::string extractFormField(const std::string &body, const std::string &field);

    static HttpResponse handleLogin(const std::string &username, const std::string &password);
    static HttpResponse handleLogout(const std::string &session);
    static HttpResponse handleRegister(const std::string &username, const std::string &password);
    static HttpResponse handleMultipart(const std::string &body, const std::string &contentType);

public:
    static std::string findContentType(const std::string &path);
    static HttpResponse handleGET(const std::string &path);
    static HttpResponse handlePOST(const std::string &path, const std::string &body, const std::string &contentType);
    static HttpResponse handleDELETE(const std::string &path);
};

#endif