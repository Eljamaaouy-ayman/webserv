#include "../includes/includes.hpp"

std::string HttpResponse::getReasonPhrase()
{
    static std::map<int, std::string> reasonPhrases = {
        {200, "OK"},
        {201, "Created"},
        {204, "No Content"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {409, "Conflict"},
        {413, "Payload Too Large"},
        {415, "Unsupported Media Type"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {505, "HTTP Version Not Supported"}};
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
        std::ifstream file(errorFilePath.c_str());
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