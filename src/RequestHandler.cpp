#include "../include/RequestHandler.hpp"

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

HttpResponse RequestHandler::handleGET(const std::string &uri)
{
    HttpResponse response;
    location *loc = getLocation(uri);

    if (loc && !loc->return_to.empty())
    {
        response.addHeader("Location", loc->return_to);
        response.setStatusCode(301);
        return response;
    }
    else if (isMethodAllowed("GET", loc) == false)
        return errorResponse(405);

    std::string index = (loc && !loc->index.empty()) ? loc->index : ConfigFile::index;
    std::string root = (loc && !loc->root.empty()) ? loc->root : ConfigFile::root;
    std::string path;
    try
    {
        path = resolvePath(uri, root);
    }
    catch (int code)
    {
        return errorResponse(code);
    }
    if (isDirectory(path))
    {
        if (uri.size() > 1 && uri[uri.size() - 1] != '/')
        {
            response.addHeader("Location", uri + '/');
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
HttpResponse RequestHandler::handlePOST(const std::string &uri, const std::string &body, const std::string &contentType)
{
    HttpResponse response;

    location *loc = getLocation(uri);
    if (loc && !loc->return_to.empty())
    {
        response.addHeader("Location", loc->return_to);
        response.setStatusCode(301);
        return response;
    }
    else if (isMethodAllowed("POST", loc) == false)
        return errorResponse(405);

    if (uri == "/login")
        return (handleLogin(extractFormField(body, "username"), extractFormField(body, "password")));
    else if (uri == "/register")
        return (handleRegister(extractFormField(body, "username"), extractFormField(body, "password")));
    else if (contentType.find("multipart/form-data") != std::string::npos)
        return handleMultipart(body, contentType);
    return errorResponse(415);

    return response;
}
HttpResponse RequestHandler::handleDELETE(const std::string &uri)
{
    HttpResponse response;
    location *loc = getLocation(uri);

    if (loc && !loc->return_to.empty())
    {
        response.addHeader("Location", loc->return_to);
        response.setStatusCode(301);
        return response;
    }
    else if (isMethodAllowed("DELETE", loc) == false)
        return errorResponse(405);
    std::string root = (loc && !loc->root.empty()) ? loc->root : ConfigFile::root;
    std::string path;
    try
    {
        path = resolvePath(uri, root);
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
