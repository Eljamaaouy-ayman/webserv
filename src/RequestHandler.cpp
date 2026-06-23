#include "../include/RequestHandler.hpp"
#include <iostream>

bool RequestHandler::isAuthenticated(HttpRequest &req)
{
    if (req.headers.find("Cookie") == req.headers.end())
        return false;
    std::string sessionID = SessionManager::extractSessionID(req.headers["Cookie"]);
    if (sessionID.empty())
        return false;
    return !SessionManager::getSessionData(sessionID).empty();
}

std::string RequestHandler::resolvePath(const std::string &uri, const std::string &root)
{
    std::string path = root + uri;
    char resolvedPath[PATH_MAX];
    if (realpath(path.c_str(), resolvedPath) == NULL)
        throw 404;
    char resolvedRoot[PATH_MAX];
    if (realpath(root.c_str(), resolvedRoot) == NULL)
        throw 404;
    std::string sResolvedPath(resolvedPath);
    std::string sResolvedRoot(resolvedRoot);
    if (sResolvedPath.compare(0, sResolvedRoot.size(), sResolvedRoot) != 0 || (sResolvedPath.size() > sResolvedRoot.size() && sResolvedPath[sResolvedRoot.size()] != '/'))
        throw 403;
    return sResolvedPath;
}

bool RequestHandler::isDirectory(const std::string &path)
{
    struct stat st;

    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool RequestHandler::isMethodAllowed(const std::string &method, location *loc)
{
    if (loc == NULL || loc->allow_methods.empty())
        return true;
    for (size_t i = 0; i < loc->allow_methods.size(); i++)
    {
        if (loc->allow_methods[i] == method)
            return true;
    }
    return false;
}

std::string RequestHandler::findContentType(const std::string &uri)
{
    size_t dotPos = uri.find_last_of('.');

    if (dotPos == std::string::npos || dotPos == 0 || uri[dotPos - 1] == '/')
        return "application/octet-stream";

    std::string extension = uri.substr(dotPos);
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

location *RequestHandler::getLocation(const std::string &uri)
{
    location *match = NULL;
    size_t matchLen = 0;
    for (size_t i = 0; i < ConfigFile::locations.size(); i++)
    {
        std::string &locPath = ConfigFile::locations[i].path;
        if (uri.compare(0, locPath.size(), locPath) == 0 && locPath.size() > matchLen)
        {
            if (uri.size() == locPath.size() || uri[locPath.size()] == '/')
            {
                match = &ConfigFile::locations[i];
                matchLen = locPath.size();
            }
        }
    }
    return match;
}

HttpResponse RequestHandler::errorResponse(int code)
{
    HttpResponse response;
    response.setStatusCode(code);
    response.setErrorPage();
    return response;
}

void RequestHandler::parseContentDisposition(Part &part)
{
    if (part.headers.find("Content-Disposition") == part.headers.end())
        throw 400;

    std::string value = part.headers["Content-Disposition"];
    if (value.find("form-data") == std::string::npos)
        throw 400;

    size_t start = value.find("name=\"");
    if (start == std::string::npos)
        throw 400;
    start += 6;
    size_t end = value.find("\"", start);
    if (end == std::string::npos)
        throw 400;
    part.name = value.substr(start, end - start);

    start = value.find("filename=\"");
    if (start == std::string::npos)
        throw 400;
    start += 10;
    end = value.find("\"", start);
    if (end == std::string::npos)
        throw 400;
    part.filename = value.substr(start, end - start);

    if (part.filename.empty())
        throw 400;
}

std::vector<Part> RequestHandler::parseMultipart(const std::string &body, const std::string &boundary)
{
    std::vector<std::string> sParts;
    std::vector<Part> parts;
    size_t start = 0;
    while (true)
    {
        start = body.find(boundary, start);
        if (start == std::string::npos)
            throw 400; // error
        start += boundary.length();
        if (body.substr(start, 2) == "--")
        {
            if (body.substr(start + 2) != "\r\n" && !body.substr(start + 2).empty())
                throw 400;
            break;
        }
        if (body.substr(start, 2) == "\r\n")
            start += 2;
        size_t end = body.find(boundary, start);
        if (end == std::string::npos)
            throw 400; // error
        std::string sPart = body.substr(start, end - start);
        if (sPart.size() >= 2 && sPart.substr(sPart.size() - 2) == "\r\n")
            sPart = sPart.substr(0, sPart.size() - 2);
        sParts.push_back(sPart);
    }
    for (size_t i = 0; i < sParts.size(); i++)
    {
        start = 0;
        Part newPart;
        size_t sep = sParts[i].find("\r\n\r\n");
        if (sep == std::string::npos)
            throw 400;
        std::string headers = sParts[i].substr(0, sep);
        newPart.body = sParts[i].substr(sep + 4);
        while (start < headers.size())
        {
            size_t end = headers.find("\r\n", start);
            std::string header = headers.substr(start, end - start);
            size_t colon = header.find(":");
            if (colon == std::string::npos)
                throw 400; // this exception was thrown i need to know why
            std::string key = header.substr(0, colon);
            std::string value = header.substr(header.find_first_not_of(" \n\t", colon + 1));
            if (key.empty() || value.empty())
                throw 400;
            newPart.headers[key] = value;
            if (end == std::string::npos)
                break;
            start = end + 2;
        }
        parseContentDisposition(newPart);
        parts.push_back(newPart);
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
HttpResponse RequestHandler::handleUpload(HttpRequest &req, location *loc)
{
    std::string root = (loc && !loc->root.empty()) ? loc->root : ConfigFile::root;
    HttpResponse response;
    try
    {
        std::string boundary = extractBoundary(req.contentType);
        std::vector<Part> parts = parseMultipart(req.body, boundary);
        if (parts.empty())
            throw 400;
        for (size_t i = 0; i < parts.size(); i++)
        {
            std::string filepath = root + "/uploads/" + parts[i].filename;
            std::ofstream file(filepath.c_str(), std::ios::binary);
            if (file.is_open() == false)
                throw 500;
            file.write(parts[i].body.c_str(), parts[i].body.size());
        }
        response.setStatusCode(201);
    }
    catch (int code)
    {
        response.setStatusCode(code);
        response.setErrorPage();
        return response;
    }
    return response;
}

HttpResponse RequestHandler::handleAutoIndex(const std::string &path, const std::string &root)
{
    DIR *dir = opendir(path.c_str());

    if (!dir)
        return errorResponse(404);
    char resolvedRoot[PATH_MAX];
    realpath(root.c_str(), resolvedRoot);

    HttpResponse response;
    response.setBody(CreatePages::AutoIndexPage(dir, path, std::string(resolvedRoot) == path));
    response.setStatusCode(200);
    response.addHeader("Content-Type", "text/html");
    closedir(dir);
    return response;
}

HttpResponse RequestHandler::handleGET(HttpRequest &req, location *loc)
{
    HttpResponse response;

    std::string index = (loc && !loc->index.empty()) ? loc->index : ConfigFile::index;
    std::string root = (loc && !loc->root.empty()) ? loc->root : ConfigFile::root;
    std::string path;
    try
    {
        path = resolvePath(req.uri, root);
    }
    catch (int code)
    {
        return errorResponse(code);
    }
    if (isDirectory(path))
    {
        if (req.uri.size() > 1 && req.uri[req.uri.size() - 1] != '/')
        {
            response.addHeader("Location", req.uri + '/');
            response.setStatusCode(301);
            return response;
        }
        if (!index.empty() && access((path + '/' + index).c_str(), F_OK) == 0)
            path += '/' + index;
        else if (loc && loc->autoindex)
            return handleAutoIndex(path, root);
        return errorResponse(403);
    }
    std::ifstream file(path.c_str(), std::ios::binary);
    if (file.is_open() == false)
        return errorResponse(errno == EACCES ? 403 : 404);
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
HttpResponse RequestHandler::handlePOST(HttpRequest &req, location *loc)
{
    HttpResponse response;

    if (req.uri == "/login")
        return (handleLogin(extractFormField(req.body, "username"), extractFormField(req.body, "password")));
    else if (req.uri == "/register")
        return (handleRegister(extractFormField(req.body, "username"), extractFormField(req.body, "password")));
    else if (req.contentType.find("multipart/form-data") != std::string::npos)
        return handleUpload(req, loc);
    return errorResponse(415);
}
HttpResponse RequestHandler::handleDELETE(HttpRequest &req, location *loc)
{
    HttpResponse response;

    std::string root = (loc && !loc->root.empty()) ? loc->root : ConfigFile::root;
    std::string path;
    try
    {
        path = resolvePath(req.uri, root);
    }
    catch (int code)
    {
        return errorResponse(code);
    }
    if (isDirectory(path))
        return errorResponse(403);
    if (std::remove(path.c_str()) != 0)
        return errorResponse(errno == EACCES ? 403 : 404);
    else
        response.setStatusCode(204);

    return response;
}
HttpResponse RequestHandler::handleRequest(HttpRequest &req)
{
    HttpResponse response;
    location *loc = getLocation(req.uri);

    if (loc && !loc->return_to.empty())
    {
        response.addHeader("Location", loc->return_to);
        response.setStatusCode(301);
        return response;
    }
    else if (isMethodAllowed(req.method, loc) == false)
        return errorResponse(405);

    if (req.uri == "/upload" && !isAuthenticated(req))
    {
        response.addHeader("Location", "/login");
        response.setStatusCode(302);
        return response;
    }
    if (req.method == "GET")
        return handleGET(req, loc);
    else if (req.method == "POST")
        return handlePOST(req, loc);
    else if (req.method == "DELETE")
        return handleDELETE(req, loc);
    return errorResponse(501);
}
