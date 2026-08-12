#include "../includes/includes.hpp"
#include <iostream>

bool RequestHandler::isAuthenticated(Request &req)
{
    if (req.request.find("Cookie") == req.request.end())
        return false;
    std::string sessionID = SessionManager::extractSessionID(req.request["Cookie"]);
    if (sessionID.empty())
        return false;
    return !SessionManager::getSessionData(sessionID).empty();
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
    if (loc == NULL || (method != "GET" && method != "POST" && method != "DELETE"))
        return true;
    for (size_t i = 0; i < loc->allow_methods.size(); i++)
    {
        if (loc->allow_methods[i] == method)
            return true;
    }
    return false;
}
std::map<std::string, std::string> RequestHandler::initMimeTypes()
{
    std::map<std::string, std::string> mimeTypes;
    mimeTypes[".html"] = "text/html";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".js"] = "text/javascript";
    mimeTypes[".txt"] = "text/plain";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".ico"] = "image/x-icon";
    mimeTypes[".svg"] = "image/svg+xml";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".pdf"] = "application/pdf";
    mimeTypes[".woff"] = "font/woff";
    mimeTypes[".woff2"] = "font/woff2";
    return mimeTypes;
}
std::string RequestHandler::getMimeType(const std::string &uri)
{
    static const std::map<std::string, std::string> mimeTypes = initMimeTypes();

    size_t dotPos = uri.find_last_of('.');

    if (dotPos == std::string::npos || dotPos == 0 || uri[dotPos - 1] == '/')
        return "application/octet-stream";

    std::string extension = uri.substr(dotPos);
    for (size_t i = 0; i < extension.length(); i++)
        extension[i] = std::tolower(extension[i]);
    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(extension);
    if (it == mimeTypes.end())
        return "application/octet-stream";
    return it->second;
}

std::string RequestHandler::urlDecode(std::string &str)
{
    std::string result;
    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] == '+')
            result += ' ';
        else if (str[i] == '%')
        {
            if (!(i + 2 < str.length() && std::isxdigit(str[i + 1]) && std::isxdigit(str[i + 2])))
                throw 400;
            result += std::strtol(str.substr(i + 1, 2).c_str(), NULL, 16);
            i += 2;
        }
        else
            result += str[i];
    }
    return result;
}

std::string RequestHandler::getFieldValue(const std::string &body, const std::string &field)
{
    size_t start = body.find(field + "=");
    if (start == std::string::npos)
        return "";
    start += field.length() + 1;
    size_t end = body.find('&', start);
    std::string value = body.substr(start, end - start);

    std::string decodedValue = urlDecode(value);

    return decodedValue;
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

location *RequestHandler::getLocation(const std::string &uri, std::vector<location> &locations)
{
    location *match = NULL;
    size_t matchLen = 0;
    for (size_t i = 0; i < locations.size(); i++)
    {
        std::string &locPath = locations[i].path;
        if (uri.compare(0, locPath.size(), locPath) == 0 && locPath.size() > matchLen)
        {
            if (uri.size() == locPath.size() || uri[locPath.size()] == '/')
            {
                match = &locations[i];
                matchLen = locPath.size();
            }
        }
    }
    return match;
}

HttpResponse RequestHandler::errorResponse(int code, ConfigFile &conf)
{
    HttpResponse response;
    response.setStatusCode(code);
    response.setErrorPage(conf);
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
            throw 400;
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
            throw 400;
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
                throw 400;
            std::string key = trim(header.substr(0, colon));
            std::string value = trim(header.substr((colon + 1)));
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
        response.setBody(CreatePages::LoginPage("Wrong username or password."));
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
HttpResponse RequestHandler::handleLogout(Request &req)
{
    HttpResponse response;
    if (req.request.find("Cookie") != req.request.end())
    {
        std::string session = SessionManager::extractSessionID(req.request["Cookie"]);
        if (!session.empty())
            SessionManager::destroySession(session);
    }
    response.setStatusCode(302);
    response.addCookie("session_id", "", "/", 0);
    response.addHeader("Location", "/login");
    return response;
}
HttpResponse RequestHandler::handleUpload(Request &req, location *loc)
{
    std::string root = (loc && !loc->root.empty()) ? loc->root : req.conf.root;
    HttpResponse response;

    std::string boundary = extractBoundary(req.request["Content-Type"]);
    std::vector<Part> parts = parseMultipart(req.request["post-body"], boundary);
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

    return response;
}

HttpResponse RequestHandler::handleAutoIndex(const std::string &path, const std::string &root)
{
    DIR *dir = opendir(path.c_str());

    if (!dir)
        throw 404;
    char resolvedRoot[PATH_MAX];
    realpath(root.c_str(), resolvedRoot);

    HttpResponse response;
    response.setBody(CreatePages::AutoIndexPage(dir, path, std::string(resolvedRoot) == path));
    response.setStatusCode(200);
    response.addHeader("Content-Type", "text/html");
    closedir(dir);
    return response;
}

HttpResponse RequestHandler::handleGET(Request &req, location *loc)
{
    HttpResponse response;

    std::string index = (loc && !loc->index.empty()) ? loc->index : req.conf.index;
    std::string root = (loc && !loc->root.empty()) ? loc->root : req.conf.root;
    std::string path;

    path = root + req.path;

    if (isDirectory(path))
    {
        if (req.path.size() > 1 && req.path[req.path.size() - 1] != '/')
        {
            response.addHeader("Location", req.path + '/');
            response.setStatusCode(301);
            return response;
        }
        if (!index.empty() && access((path + '/' + index).c_str(), F_OK) == 0)
            path += '/' + index;
        else if (loc && loc->autoindex)
            return handleAutoIndex(path, root);
        else
            throw 400;
    }
    if (access(path.c_str(), F_OK) != 0)
        throw 404;
    std::ifstream file(path.c_str(), std::ios::binary);
    if (file.is_open() == false)
        throw 403;
    else
    {
        std::ostringstream buffer;
        buffer << file.rdbuf();
        response.setBody(buffer.str());
        response.setStatusCode(200);
        response.addHeader("Content-Type", getMimeType(path));
    }

    return response;
}
HttpResponse RequestHandler::handlePOST(Request &req, location *loc)
{
    HttpResponse response;

    if (req.path == "/login")
        return (handleLogin(getFieldValue(req.request["post-body"], "username"), getFieldValue(req.request["post-body"], "password")));
    else if (req.path == "/register")
        return (handleRegister(getFieldValue(req.request["post-body"], "username"), getFieldValue(req.request["post-body"], "password")));
    else if (req.request["Content-Type"].find("multipart/form-data") != std::string::npos)
        return handleUpload(req, loc);
    else if (req.path == "/logout")
        return handleLogout(req);
    return errorResponse(415, req.conf);
}
HttpResponse RequestHandler::handleDELETE(Request &req, location *loc)
{
    HttpResponse response;

    std::string root = (loc && !loc->root.empty()) ? loc->root : req.conf.root;
    std::string path;

    path = root + req.path;
    if (isDirectory(path))
        throw 403;
    else if (access(path.c_str(), F_OK) != 0)
        throw 404;
    else if (std::remove(path.c_str()) != 0)
        throw 403;
    else
        response.setStatusCode(204);

    return response;
}
HttpResponse RequestHandler::handleRequest(Request &req)
{
    if (req.method == "ERROR")
        return errorResponse(400, req.conf);

    HttpResponse response;
    location *loc = getLocation(req.path, req.conf.locations);
    if (req.request["post-body"].size() > static_cast<size_t>(req.conf.client_max_size_body))
        return errorResponse(413, req.conf);

    if (loc && !loc->return_to.empty())
    {
        response.addHeader("Location", loc->return_to);
        response.setStatusCode(301);
        return response;
    }
    else if (isMethodAllowed(req.method, loc) == false)
        return errorResponse(405, req.conf);

    if (req.path == "/upload.html" && !isAuthenticated(req))
    {
        response.addHeader("Location", "/login.html");
        response.setStatusCode(302);
        return response;
    }
    try
    {
        // std::cout << "requested path: " << req.path << std::endl;
        if (req.method == "GET")
            return handleGET(req, loc);
        else if (req.method == "POST")
            return handlePOST(req, loc);
        else if (req.method == "DELETE")
            return handleDELETE(req, loc);
    }
    catch (int e)
    {
        return errorResponse(e, req.conf);
    }
    return errorResponse(501, req.conf);
}
