#include "../includes/includes.hpp"

std::map<int, std::string> HttpResponse::initReasonPhrases()
{
    std::map<int, std::string> reasonPhrases;
    reasonPhrases[200] = "OK";
    reasonPhrases[201] = "Created";
    reasonPhrases[204] = "No Content";
    reasonPhrases[301] = "Moved Permanently";
    reasonPhrases[302] = "Found";
    reasonPhrases[400] = "Bad Request";
    reasonPhrases[401] = "Unauthorized";
    reasonPhrases[403] = "Forbidden";
    reasonPhrases[404] = "Not Found";
    reasonPhrases[405] = "Method Not Allowed";
    reasonPhrases[409] = "Conflict";
    reasonPhrases[413] = "Payload Too Large";
    reasonPhrases[415] = "Unsupported Media Type";
    reasonPhrases[500] = "Internal Server Error";
    reasonPhrases[501] = "Not Implemented";
    reasonPhrases[504] = "Gateway Timeout";
    reasonPhrases[505] = "HTTP Version Not Supported";
    return reasonPhrases;
}

std::string HttpResponse::getReasonPhrase()
{
    static std::map<int, std::string> reasonPhrases = initReasonPhrases();

    std::map<int, std::string>::const_iterator it = reasonPhrases.find(statusCode);
    if (it == reasonPhrases.end())
        return "Unknown";
    return it->second;
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

void HttpResponse::setErrorPage(ConfigFile &conf)
{
    std::ostringstream page;

    if (conf.error_page.find(statusCode) != conf.error_page.end())
    {
        std::string errorFilePath = conf.error_page[statusCode];
        std::ifstream file((conf.root + "/" + errorFilePath.c_str()).c_str());
        if (file.is_open())
        {
            page << file.rdbuf();
            addHeader("Content-Type", RequestHandler::getMimeType(errorFilePath));
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

    response << "HTTP/1.1 " << statusCode << ' ' << getReasonPhrase() << "\r\n";
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++)
        response << it->first << ": " << it->second << "\r\n";
    for (size_t i = 0; i < cookies.size(); i++)
        response << "Set-Cookie: " << cookies[i] << "\r\n";
    response << "Content-Length: " << body_.size() << "\r\n\r\n";
    response << body_;
    return response.str();
}