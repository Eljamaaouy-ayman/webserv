#include "../include/RequestHandler.hpp"

std::string RequestHandler::findContentType(const std::string &path)
{
    size_t dotPos = path.find_last_of('.');

    if (dotPos == std::string::npos || dotPos == 0 || path[dotPos - 1] == '/')
        return "application/octet-stream";

    std::string extension = path.substr(dotPos);
    for (size_t i = 0; i < extension.length(); i++)
        extension[i] = std::tolower(extension[i]);
    if (extension == ".html" || extension == ".htm")
        return "text/html";
    if (extension == ".css")
        return "text/css";
    if (extension == ".js")
        return "application/javascript";
    if (extension == ".txt")
        return "text/plain";
    if (extension == ".png")
        return "image/png";
    if (extension == ".jpeg")
        return "image/jpeg";
    if (extension == ".gif")
        return "image/gif";
    return "application/octet-stream";
}

std::string RequestHandler::extractFormField(const std::string &body, const std::string &field)
{
    size_t start = body.find(field + "=");
    if (start == std::string::npos)
        return "";
    start += field.length() + 1;
    size_t end = body.find('&', start);
    std::string str = body.substr(start, end - start);

    std::string extractedField;
    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] == '+')
            extractedField += ' ';
        else if (str[i] == '%' && i + 2 < str.length())
        {
            extractedField += std::strtol(str.substr(i + 1, 2).c_str(), NULL, 16);
            i += 2;
        }
        else
            extractedField += str[i];
    }
    return extractedField;
}

std::string RequestHandler::extractBoundary(const std::string &contentType)
{
    size_t boundaryKeyPos = contentType.find("boundary=");
    if (boundaryKeyPos == std::string::npos)
        return "";
    size_t start = boundaryKeyPos + 9;
    size_t end = contentType.find(';', start);
    std::string boundary = contentType.substr(start, end - start);
    if (boundary.length() >= 2 && boundary[0] == '"' && boundary[boundary.length() - 1] == '"')
        boundary = boundary.substr(1, boundary.length() - 2);
    return "--" + boundary;
}
void RequestHandler::parseMultipartHeaders(const std::string &headers)
{
    size_t start = 0;
    bool hasContentType = false;
    bool hasContentDisposition = false;
    while (start < headers.size())
    {
        size_t end = headers.find("\r\n", start);
        std::string header = headers.substr(start, end - start);
        size_t colon = header.find(": ");
        if (colon == std::string::npos)
            throw 400;
        std::string key = header.substr(0, colon);
        std::string value = header.substr(colon + 2);

        if (key.empty() || value.empty())
            throw 400;
        if (key == "Content-Disposition")
        {
            if (hasContentDisposition)
                throw 400;
            size_t pos = value.find("filename=\"");
            if (value.find(" form-data; name=\"") != 0 || pos == std::string::npos)
                throw 400;
            size_t filenameEnd = value.find('\"', pos + 10);
            if (filenameEnd == std::string::npos || filenameEnd == pos + 10)
                throw 400;
            hasContentDisposition = true;
        }
        else if (key == "Content-Type")
        {
            if (hasContentType)
                throw 400;
            hasContentType = true;
        }
        if (end == std::string::npos)
            break;
        start = end + 2;
    }
    if (!hasContentDisposition)
        throw 400;
}

std::vector<std::string> RequestHandler::parseMultipart(const std::string &body, const std::string &boundary)
{
    std::vector<std::string> parts;
    size_t start = 0;
    while (true)
    {
        start = body.find(boundary, start);
        if (start == std::string::npos)
            throw 400; // error
        start += boundary.length();
        if (body.substr(start, 2) == "--")
        {
            if (body.substr(start + 2) != "\r\n" && body.substr(start + 2) != "")
                throw 400;
            break;
        }
        if (body.substr(start, 2) == "\r\n")
            start += 2;
        size_t end = body.find(boundary, start);
        if (end == std::string::npos)
            throw 400; // error
        std::string part = body.substr(start, end - start);
        if (part.size() >= 2 && part.substr(part.size() - 2) == "\r\n")
            part = part.substr(0, part.size() - 2);
        parts.push_back(part);
    }
    for (size_t i = 0; i < parts.size(); i++)
    {
        size_t sep = parts[i].find("\r\n\r\n");
        if (sep == std::string::npos)
            throw 400;
        std::string headers = parts[i].substr(0, sep);
        if (headers.empty())
            throw 400;
        parseMultipartHeaders(headers);
    }
    return parts;
}

HttpResponse RequestHandler::handleLogin(const std::string &username, const std::string &password)
{
    HttpResponse response;
    if (UserManager::authenticateUser(username, password) == false)
    {
        response.setStatusCode(401);
        response.setBody(CreatePages::LoginPage("Incorrect username or password."));
        response.addHeader("Content-Type", "text/html");
    }
    else
    {
        response.setStatusCode(302);
        std::string session_id = SessionManager::createSession(username);
        response.addCookie("session_id", session_id, "/");
        response.addHeader("Location", "/");
    }

    return response;
}

HttpResponse RequestHandler::handleRegister(const std::string &username, const std::string &password)
{
    HttpResponse response;
    if (UserManager::addUser(username, password) == false)
    {
        response.setStatusCode(409);
        response.setBody(CreatePages::RegisterPage("Username already exists"));
        response.addHeader("Content-Type", "text/html");
    }
    else
    {
        response.setStatusCode(302);
        std::string session_id = SessionManager::createSession(username);
        response.addCookie("session_id", session_id, "/");
        response.addHeader("Location", "/");
    }
    return response;
}
HttpResponse RequestHandler::handleLogout(const std::string &session)
{
    HttpResponse response;
    SessionManager::destroySession(session);

    response.setStatusCode(302);
    response.addCookie("session_id", "", "/", 0);
    response.addHeader("Location", "/login");
    return response;
}
HttpResponse RequestHandler::handleMultipart(const std::string &body, const std::string &contentType)
{
    HttpResponse response;
    try
    {
        std::string boundary = extractBoundary(contentType);
        std::vector<std::string> parts = parseMultipart(body, boundary);
        for (size_t i = 0; i < parts.size(); i++)
        {
            std::vector<std::string> headers;
            size_t filenameStart = parts[i].find("filename=\"") + 10;
            size_t filenameEnd = parts[i].find('"', filenameStart);
            std::string filename = parts[i].substr(filenameStart, filenameEnd - filenameStart);
            if (filename.find_first_of("/\\") != std::string::npos)
                throw 400;
            size_t contentStart = parts[i].find("\r\n\r\n") + 4;
            std::string content = parts[i].substr(contentStart);
            std::string filepath = "www/uploads/" + filename;
            std::ofstream file(filepath.c_str(), std::ios::binary);
            if (file.is_open() == false)
                throw 500;
            file.write(content.c_str(), content.size());
            response.setStatusCode(201);
        }
    }
    catch (int e)
    {
        response.setStatusCode(e);
        response.setErrorPage();
        return response;
    }
    return response;
}

HttpResponse RequestHandler::handleGET(const std::string &path)
{
    std::ifstream file((ConfigFile::root + path).c_str(), std::ios::binary);
    HttpResponse response;

    if (file.is_open() == false)
    {
        response.setStatusCode(404);
        response.setErrorPage();
    }
    else
    {
        std::ostringstream buffer;
        buffer << file.rdbuf();
        response.setBody(buffer.str());
        response.setStatusCode(200);
        response.addHeader("Content-Type", findContentType(path));
    }
    return response;
}
HttpResponse RequestHandler::handlePOST(const std::string &path, const std::string &body, const std::string &contentType)
{
    HttpResponse response;

    if (path == "/login")
        return (handleLogin(extractFormField(body, "username"), extractFormField(body, "password")));
    else if (path == "/register")
        return (handleRegister(extractFormField(body, "username"), extractFormField(body, "password")));
    else if (contentType.find("multipart/form-data") != std::string::npos)
    {
        return handleMultipart(body, contentType);
        // std::string boundary = extractBoundary(contentType);
        // std::vector<std::string> parts = splitMultipart(body, boundary);
        // for (size_t i = 0; i < parts.size(); i++)
        // {
        //     size_t filenameStart = parts[i].find("filename=\"") + 10;
        //     size_t filenameEnd = parts[i].find('"', filenameStart);
        //     std::string filename = parts[i].substr(filenameStart, filenameEnd - filenameStart);

        //     size_t contentStart = parts[i].find("\r\n\r\n") + 4;
        //     std::string content = parts[i].substr(contentStart);
        //     std::string filepath = "www/uploads/" + filename;
        //     std::ofstream file(filepath.c_str(), std::ios::binary);
        //     if (file.is_open() == false)
        //     {
        //         response.setStatusCode(500);
        //         response.setErrorPage();
        //     }
        //     file.write(content.c_str(), content.size());
        //     response.setStatusCode(201);
        // }
    }
    return response;
}
HttpResponse RequestHandler::handleDELETE(const std::string &path)
{
    HttpResponse response;

    if (std::remove(path.c_str()) != 0)
    {
        response.setStatusCode(403);
        response.setErrorPage();
    }
    else
        response.setStatusCode(204);

    return response;
}
