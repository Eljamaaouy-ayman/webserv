#include "../include/HttpResponse.hpp"

std::string HttpResponse::getReasonPhrase()
{
    switch (statusCode)
    {
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 204:
        return "No Content";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 400:
        return "Bad Request";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 409:
        return "Conflict";
    case 413:
        return "Payload Too Large";
    case 415:
        return "Unsupported Media Type";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 505:
        return "HTTP Version Not Supported";
    default:
        return "Unknown";
    }

    return "Unknown";
}

void HttpResponse::setStatusCode(int code)
{
    statusCode = code;
}
void HttpResponse::addHeader(const std::string &key, const std::string &value)
{
    headers[key] = value;
}
void HttpResponse::setBody(const std::string &body)
{
    body_ = body;
}

void HttpResponse::addCookie(const std::string &key, const std::string &value, const std::string &path)
{
    std::ostringstream cookie;

    cookie << key << '=' << value << "; Path=" << path;
    cookies.push_back(cookie.str());
}

void HttpResponse::addCookie(const std::string &key, const std::string &value, const std::string &path, int maxAge)
{
    std::ostringstream cookie;

    cookie << key << '=' << value << "; Path=" << path << "; Max-Age=" << maxAge;
    cookies.push_back(cookie.str());
}

void HttpResponse::setErrorPage()
{
    std::ostringstream page;

    if (ConfigFile::error_page.find(statusCode) != ConfigFile::error_page.end())
    {
        std::string errorFilePath = ConfigFile::error_page[statusCode];
        std::ifstream file(errorFilePath.c_str());
        if (file.is_open())
        {
            page << file.rdbuf();
            addHeader("Content-Type", RequestHandler::findContentType(errorFilePath));
            body_ = page.str();
            return;
        }
    }
    page << "<!doctype html>\n"
         << "<html>\n"
         << "<head>\n"
         << "<title>"
         << statusCode
         << "</title>"
         << "\n</head>\n"
         << "<body>\n"
         << "<h1>"
         << getReasonPhrase()
         << "</h1>\n"
         << "</body>\n"
         << "</html>\n";

    addHeader("Content-Type", "text/html");
    body_ = page.str();
}

std::string HttpResponse::build()
{
    std::ostringstream response;

    response << "HTTP/1.1 " << statusCode << ' ' << getReasonPhrase() << '\n';
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++)
        response << it->first << ": " << it->second << '\n';
    for (size_t i = 0; i < cookies.size(); i++)
        response << "Set-Cookie: " << cookies[i] << '\n';
    response << "Content-Length: " << body_.size() << "\n\n";
    response << body_;
    return response.str();
}