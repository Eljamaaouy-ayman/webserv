#include "../includes/request.hpp"

Request::Request() : method("ELSE"), path(""), httpV(""), isCGI(false), _foundCookie(false)
{
  // generate a session_id for cookies
  srand(time(0));
  int sessionId = rand();

  std::stringstream ss;
  ss << sessionId;
  this->setSession("session_id", ss.str());
}

// Default Constructor of struct CgiInfo
Request::CgiInfo::CgiInfo()
    : host(""), port(""), method("GET"), scriptPath(""), pathInfo(""),
      query(""), body(""), contentLength(0), contentType("") {}

/**
 * Main entry point - parses the complete HTTP request
 */
void Request::setRequest(const std::string &req) {
    // Clear all existing data
    clearRequestData();
    
    // Parse the request line (method, path, HTTP version)
    std::string line;
    std::stringstream ss(req);
    if (std::getline(ss, line)) {
        if (!parseRequestLine(line)) {
            this->method = "ERROR";
            return;
        }
        // Check if request is CGI
        this->checkCGI(this->path);
    }
    
    // Parse headers
    if (!parseHeaders(req)) {
        this->method = "ERROR";
        return;
    }
    
    // Validate required headers (Host, Content-Length for POST)
    validateAndStoreRequest();
    
    // Parse body if exists
    parseBody(req);
    
    // Handle multipart/form-data (file uploads)
    processMultipartBody();
    
    // Remove Cookie if not present in request
    if (!_foundCookie) {
        this->request.erase("Cookie");
    }
    
    // Setup CGI info if this is a CGI request
    if (this->getIsCGI()) {
        setupCgiInfo();
    }

    // print request for debugging

    std::cout << "Request Method: " << this->method << std::endl;
    std::cout << "Request Path: " << this->path << std::endl;
    std::cout << "Request HTTP Version: " << this->httpV << std::endl;
    std::cout << "Request Headers: " << std::endl;
    std::cout << "------------------------" << std::endl;
    for (const auto& pair : request) {
        std::cout << "Key: " << pair.first << " => Value: " << pair.second << std::endl;
    }
    std::cout << "------------------------" << std::endl;
    for (const auto& pair : session) {
    std::cout << "Key: " << pair.first << " => Value: " << pair.second << std::endl;
    }
    std::cout << "------------------------" << std::endl;
    std::cout << "Is CGI Request: " << (this->getIsCGI() ? "Yes" : "No") << std::endl;
    std::cout << "------------------------" << std::endl;
    std::cout << "CGI Info: " << std::endl;
    std::cout << "Host: " << this->cgi.host << std::endl;
    std::cout << "Port: " << this->cgi.port << std::endl;
    std::cout << "Method: " << this->cgi.method << std::endl;
    std::cout << "Script Path: " << this->cgi.scriptPath << std::endl;
    std::cout << "Path Info: " << this->cgi.pathInfo << std::endl;
    std::cout << "Query: " << this->cgi.query << std::endl;
    for (const auto& pair : cgi.headers) {
        std::cout << "Key: " << pair.first << " => Value: " << pair.second << std::endl;
    }
    std::cout << "Body: " << this->cgi.body << std::endl;
    std::cout << "Content Length: " << this->cgi.contentLength << std::endl;
    std::cout << "Content Type: " << this->cgi.contentType << std::endl;
    std::cout << "------------------------" << std::endl;
    std::cout << "Request Parsing Completed" << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << std::endl;
}

/**
 * Clears all request data before parsing a new request
 */
void Request::clearRequestData() {
    // Clear main request data
    if (!this->request.empty()) {
        this->request.clear();
    }
    if (!this->path.empty()) {
        this->path.clear();
    }
    if (!this->httpV.empty()) {
        this->httpV.clear();
    }
    
    // Clear CGI data
    if (!this->cgi.host.empty()) this->cgi.host.clear();
    if (!this->cgi.port.empty()) this->cgi.port.clear();
    if (!this->cgi.method.empty()) this->cgi.method.clear();
    if (!this->cgi.scriptPath.empty()) this->cgi.scriptPath.clear();
    if (!this->cgi.pathInfo.empty()) this->cgi.pathInfo.clear();
    if (!this->cgi.query.empty()) this->cgi.query.clear();
    if (!this->cgi.headers.empty()) this->cgi.headers.clear();
    if (!this->cgi.body.empty()) this->cgi.body.clear();
    if (!this->cgi.contentType.empty()) this->cgi.contentType.clear();
    this->cgi.contentLength = 0;
    
    _foundCookie = false;
}

/**
 * Parses the first line of HTTP request (method, path, version)
 * Returns true if parsing was successful
 */
bool Request::parseRequestLine(const std::string& line) {
    std::stringstream firstLine(line);
    std::string reqMethod;
    
    // Extract method
    firstLine >> reqMethod;
    if (reqMethod == "GET") {
        this->method = "GET";
    } else if (reqMethod == "POST") {
        this->method = "POST";
    } else if (reqMethod == "DELETE") {
        this->method = "DELETE";
    } else {
        this->method = "ERROR";
        return false;
    }
    
    // Extract path and HTTP version
    firstLine >> this->path >> this->httpV;
    
    // Validate HTTP version
    if (this->httpV != "HTTP/1.0" && this->httpV != "HTTP/1.1") {
        this->method = "ERROR";

        return false;
    }
    
    return true;
}

/**
 * Parses all headers from the request
 * Returns true if headers are valid
 */
bool Request::parseHeaders(const std::string& req) {
    std::stringstream ss(req);
    std::string line;
    size_t pos;
    
    // Skip the request line (first line)
    std::getline(ss, line);
    
    // Parse headers
    while (std::getline(ss, line)) {
        pos = line.find(":");
        
        if (pos == std::string::npos) {
            // Empty line means end of headers
            if (line == "\r" || line.empty()) {
                break;
            }
            return false;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 2); // Skip ": "
        
        // Remove trailing \r if present
        if (!value.empty() && value.back() == '\r') {
            value.pop_back();
        }
        
        // Check if key or value is empty
        if (key.empty() || value.empty()) {
            return false;
        }
        
        this->request[key] = value;
    }
    
    return true;
}

/**
 * Validates required headers and stores them
 */
void Request::validateAndStoreRequest() {
    // Validate Host header
    auto itHost = this->request.find("Host");
    if (itHost == this->request.end()) {
        this->method = "ERROR";

        return;
    }
    
    // Parse host and port from Host header
    std::string hostValue = itHost->second;
    size_t posHost = hostValue.find(":");
    
    if (posHost != std::string::npos) {
        std::string domainName = hostValue.substr(0, posHost);
        if (domainName != "localhost" && domainName != this->conf.host) {
            this->method = "ERROR";
            return;
        }
        
        int reqListen = std::atoi(hostValue.substr(posHost + 1).c_str());
        auto it = std::find(this->conf.listen.begin(), 
                           this->conf.listen.end(), reqListen);
        if (it == this->conf.listen.end()) {
            this->method = "ERROR";

            return;
        }
    } else {
        this->method = "ERROR";
        return;
    }
    
    // Validate Content-Length for POST requests
    if (this->method == "POST") {
        auto itContentLength = this->request.find("Content-Length");
        if (itContentLength == this->request.end()) {
            this->method = "ERROR";
            return;
        }
        int contentLengthValue = std::atoi(itContentLength->second.c_str());
        if (contentLengthValue < 0) {
            this->method = "ERROR";
            return;
        }
    }
    
    // Check if client sent cookies
    if (this->request.find("Cookie") != this->request.end()) {
        _foundCookie = true;
    }
}

/**
 * Parses the request body and stores it
 */
void Request::parseBody(const std::string& req) {
    size_t pos = req.find("\r\n\r\n");
    if (pos == std::string::npos) {
        return;
    }
    
    std::string body = req.substr(pos + 4);
    if (!body.empty()) {
        this->request["post-body"] = body;
    }
}

/**
 * Processes multipart/form-data for file uploads
 * Extracts filename and binary data
 */
void Request::processMultipartBody() {
    // Check if content type is multipart/form-data
    std::string contentType = this->request.count("Content-Type") 
                              ? this->request["Content-Type"] 
                              : "";
    
    if (contentType.substr(0, 52) != "multipart/form-data; boundary=----WebKitFormBoundary") {
        return;
    }
    
    std::string& postBody = this->request["post-body"];
    
    // Find Content-Disposition
    size_t pos = postBody.find("Content-Disposition: ");
    if (pos == std::string::npos) {
        std::cerr << "Content-Disposition not found" << std::endl;
        return;
    }
    
    // Find filename
    pos = postBody.find("filename=\"", pos);
    if (pos == std::string::npos) {
        std::cerr << "filename not found" << std::endl;
        return;
    }
    
    // Extract filename
    size_t start = pos + 10;
    size_t end = postBody.find("\"", start);
    std::string filename = postBody.substr(start, end - start);
    this->request["filename"] = filename;
    
    // Find binary data start
    pos = postBody.find("\r\n\r\n", end);
    if (pos == std::string::npos) {
        std::cerr << "binary data not found" << std::endl;
        return;
    }
    
    // Find last boundary
    end = postBody.rfind("\r\n------WebKitFormBoundary");
    if (end == std::string::npos) {
        std::cerr << "last boundary not found" << std::endl;
        return;
    }
    
    // Extract binary data
    start = pos + 4;
    std::string binaryData = postBody.substr(start, end - start);
    
    if (!binaryData.empty()) {
        this->request["binary-data"] = binaryData;
    } else {
        this->request.erase("binary-data");
    }
}

/**
 * Sets up CGI information if request is a CGI request
 */
void Request::setupCgiInfo() {
    // Copy host
    auto hostIt = this->request.find("Host");
    if (hostIt != this->request.end()) {
        std::string hostValue = hostIt->second;
        size_t colonPos = hostValue.find(":");
        this->cgi.host = (colonPos != std::string::npos) 
                         ? hostValue.substr(0, colonPos) 
                         : "";
        this->cgi.port = (colonPos != std::string::npos) 
                         ? hostValue.substr(colonPos + 1) 
                         : "";
    }
    
    // Copy method
    this->cgi.method = this->getMethodByName(this->method);
    
    // Copy headers
    this->cgi.headers = this->request;
    
    // Copy body
    auto bodyIt = this->request.find("post-body");
    this->cgi.body = (bodyIt != this->request.end()) ? bodyIt->second : "";
    
    // Copy content length
    auto contentLengthIt = this->request.find("Content-Length");
    this->cgi.contentLength = (contentLengthIt != this->request.end())
                              ? std::strtol(contentLengthIt->second.c_str(), NULL, 10)
                              : 0;
    
    // Copy content type
    auto contentTypeIt = this->request.find("Content-Type");
    this->cgi.contentType = (contentTypeIt != this->request.end()) 
                            ? contentTypeIt->second 
                            : "";
}

// Add a private member variable for tracking cookies
// In Request.hpp add: bool _foundCookie;


const std::map<std::string, std::string> &Request::getRequest() const {
  return this->request;
}

// Session

void Request::setSession(const std::string session_id,
                         const std::string value) {
  this->session[session_id] = value;
}

const std::map<std::string, std::string> &Request::getSession() const {
  return this->session;
}

void Request::checkCGI(std::string path) {
  this->isCGI = false;

  if (!path.empty() && path[0] == '/') {
    path.erase(0, 1);
  }

  // check the request is cgi
  size_t pos = path.find("/");
  if (pos == std::string::npos || path.substr(0, pos) != "cgi-bin") {
    std::cout << "Request is not CGI" << std::endl;
    return;
  }

  // check folder cgi-bin is in root folder
  std::ifstream cgiFolder(
      (this->conf.root + "/" + path.substr(0, pos)).c_str());
  if (!cgiFolder.is_open()) {
    return;
  }

  // find script path and queries
  size_t posPathInfo = path.find("/", pos + 1);
  pos = path.find("?");

  // First, handle the script path
  if (posPathInfo != std::string::npos) {
    this->cgi.scriptPath =
        this->conf.root + "/" + path.substr(0, posPathInfo);
  } else if (pos != std::string::npos) {
    this->cgi.scriptPath = this->conf.root + "/" + path.substr(0, pos);
  } else {
    this->cgi.scriptPath = this->conf.root + "/" + path;
  }

  // Then, handle path info (only the path part before ?)
  if (posPathInfo != std::string::npos) {
    size_t endOfPathInfo = (pos != std::string::npos) ? pos : path.length();
    this->cgi.pathInfo = path.substr(posPathInfo, endOfPathInfo - posPathInfo);
  }

  // Finally, handle queries
  if (pos != std::string::npos) {
    this->cgi.query = path.substr(pos + 1);
  }

  // check if file is in cgi-bin folder
  if (access((this->cgi.scriptPath).c_str(), F_OK) == -1 ||
      !this->pathGCIisFile(this->cgi.scriptPath)) {
    this->isCGI = false;
    return;
  }

  this->isCGI = true;
}

const bool &Request::getIsCGI() const { return this->isCGI; }

// check if a file
bool Request::pathGCIisFile(std::string path) {
  struct stat buffer;

  // get all status of path (size, type, ...)
  if (stat(path.c_str(), &buffer) == -1) {
    return false;
  }

  // check the path is file or not
  if (S_ISREG(buffer.st_mode)) {
    return true;
  }

  return false;
}

void Request::setCgiResponse(const std::string &cgiResponse) {
  _cgiResponse = cgiResponse;
}

const std::string &Request::getCgiResponse() const { return _cgiResponse; }

std::string Request::getMethodByName(std::string methodName)
{
    if(methodName == "GET")
        return "GET";
    else if(methodName == "POST")
        return "POST";
    else if(methodName == "DELETE")
        return "DELETE";
    else
        return "NOT ALLOWED METHOD";
}
