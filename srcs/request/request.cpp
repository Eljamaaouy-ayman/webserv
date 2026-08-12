#include "../includes/request.hpp"

Request::Request() : method("ELSE"), path(""), httpV(""), isCGI(false), _foundCookie(false)
{
  srand(time(0));
  int sessionId = rand();

  std::stringstream ss;
  ss << sessionId;
  this->setSession(ss.str());
}

Request::CgiInfo::CgiInfo()
    : host(""), port(""), method("GET"), scriptPath(""), pathInfo(""),
      query(""), body(""), contentLength(0), contentType("") {}

void Request::setRequest(const std::string &req) {
    // Clear all existing data
    clearRequestData();
    
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

    if (!parseHeaders(req)) {
        this->method = "ERROR";
        return;
    }

    validateAndStoreRequest();

    parseBody(req);
    
    processMultipartBody();
    // for (const auto& pair : request) {
    //     std::cout << "Key: " << pair.first << std::endl;
    // }
    
    if (!_foundCookie) {
        this->request.erase("Cookie");
    }
    
    if (this->getIsCGI()) {
        setupCgiInfo();
    }
}

void Request::clearRequestData() {
    if (!this->request.empty()) {
        this->request.clear();
    }
    if (!this->path.empty()) {
        this->path.clear();
    }
    if (!this->httpV.empty()) {
        this->httpV.clear();
    }
    
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

bool Request::parseRequestLine(const std::string& line) {
    std::stringstream firstLine(line);
    std::string reqMethod;
    
    firstLine >> reqMethod;
        this->method = reqMethod;
    
    firstLine >> this->path >> this->httpV;
    
    if (this->httpV != "HTTP/1.0" && this->httpV != "HTTP/1.1") {
        this->method = "ERROR";
        return false;
    }
    return true;
}

bool Request::parseHeaders(const std::string& req) {
    std::stringstream ss(req);
    std::string line;
    size_t pos;
    
    std::getline(ss, line);

    while (std::getline(ss, line)) {
        pos = line.find(":");
        
        if (pos == std::string::npos) {
            if (line == "\r" || line.empty()) {
                break;
            }
            return false;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 2); 
        
        // if (!value.empty() && value.back() == '\r') {
        //     value.pop_back();
        // }

        if (!value.empty() && value[value.size() - 1] == '\r') {
            value.erase(value.size() - 1);
        }
        
        if (key.empty() || value.empty()) {
            return false;
        }
        
        this->request[key] = value;
    }
    
    return true;
}

void Request::validateAndStoreRequest() {
    std::map<std::string, std::string>::iterator itHost = this->request.find("Host");
    if (itHost == this->request.end()) {
        this->method = "ERROR";
        return;
    }
    
    std::string hostValue = itHost->second;
    size_t posHost = hostValue.find(":");
    
    if (posHost != std::string::npos) {
        std::string domainName = hostValue.substr(0, posHost);
        if (domainName != "localhost" && domainName != this->conf.host) {
            this->method = "ERROR";
            return;
        }

        int reqListen = std::atoi(hostValue.substr(posHost + 1).c_str());
        std::vector<int>::iterator it = std::find(this->conf.listen.begin(),
                           this->conf.listen.end(), reqListen);
        if (it == this->conf.listen.end()) {
            this->method = "ERROR";
            return;
        }
    } else {
        this->method = "ERROR";
        return;
    }
    
    if (this->method == "POST") {
        std::map<std::string, std::string>::iterator itContentLength = this->request.find("Content-Length");
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
    
    if (this->request.find("Cookie") != this->request.end()) {
        _foundCookie = true;
    }
}

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

void Request::processMultipartBody() {
    std::string contentType = this->request.count("Content-Type") 
                              ? this->request["Content-Type"] 
                              : "";
    
    if (contentType.substr(0, 52) != "multipart/form-data; boundary=----WebKitFormBoundary") {
        return;
    }
    
    std::string& postBody = this->request["post-body"];
    
    size_t pos = postBody.find("Content-Disposition: ");
    if (pos == std::string::npos) {
        std::cerr << "Content-Disposition not found" << std::endl;
        return;
    }
    
    pos = postBody.find("filename=\"", pos);
    if (pos == std::string::npos) {
        std::cerr << "filename not found" << std::endl;
        return;
    }
    
    size_t start = pos + 10;
    size_t end = postBody.find("\"", start);
    std::string filename = postBody.substr(start, end - start);
    this->request["filename"] = filename;
    
    pos = postBody.find("\r\n\r\n", end);
    if (pos == std::string::npos) {
        std::cerr << "binary data not found" << std::endl;
        return;
    }
    
    end = postBody.rfind("\r\n------WebKitFormBoundary");
    if (end == std::string::npos) {
        std::cerr << "last boundary not found" << std::endl;
        return;
    }
    
    start = pos + 4;
    std::string binaryData = postBody.substr(start, end - start);
    
    if (!binaryData.empty()) {
        this->request["binary-data"] = binaryData;
    } else {
        this->request.erase("binary-data");
    }

}

void Request::setupCgiInfo() {
    std::map<std::string, std::string>::iterator hostIt = this->request.find("Host");
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
    
    this->cgi.method = this->getMethodByName(this->method);
    
    this->cgi.headers = this->request;
    
    std::map<std::string, std::string>::iterator bodyIt = this->request.find("post-body");
    this->cgi.body = (bodyIt != this->request.end()) ? bodyIt->second : "";
    
    std::map<std::string, std::string>::iterator contentLengthIt = this->request.find("Content-Length");
    this->cgi.contentLength = (contentLengthIt != this->request.end())
                              ? std::strtol(contentLengthIt->second.c_str(), NULL, 10)
                              : 0;
    
    std::map<std::string, std::string>::iterator contentTypeIt = this->request.find("Content-Type");
    this->cgi.contentType = (contentTypeIt != this->request.end()) 
                            ? contentTypeIt->second 
                            : "";
}

const std::map<std::string, std::string> &Request::getRequest() const {
  return this->request;
}

void Request::setSession(
                         const std::string value) {
  this->session["session_id"] = value;
}

const std::map<std::string, std::string> &Request::getSession() const {
  return this->session;
}

void Request::checkCGI(std::string path) {
  this->isCGI = false;

  if (!path.empty() && path[0] == '/') {
    path.erase(0, 1);
  }

  size_t pos = path.find("/");
  if (pos == std::string::npos || path.substr(0, pos) != "cgi-bin") {
    return;
  }

  std::ifstream cgiFolder((this->conf.root + "/" + path.substr(0, pos)).c_str());
  if (!cgiFolder.is_open()) {
    return;
  }


  size_t posPathInfo = path.find("/", pos + 1);
  pos = path.find("?");

  if (posPathInfo != std::string::npos) {
    this->cgi.scriptPath = this->conf.root + "/" + path.substr(0, posPathInfo);
  } else if (pos != std::string::npos) {
    this->cgi.scriptPath = this->conf.root + "/" + path.substr(0, pos);
  } else {
    this->cgi.scriptPath = this->conf.root + "/" + path;
  }

  if (posPathInfo != std::string::npos) {
    size_t endOfPathInfo = (pos != std::string::npos) ? pos : path.length();
    this->cgi.pathInfo = path.substr(posPathInfo, endOfPathInfo - posPathInfo);
  }

  if (pos != std::string::npos) {
    this->cgi.query = path.substr(pos + 1);
  }

  if (access((this->cgi.scriptPath).c_str(), F_OK) == -1 ||
      !this->pathGCIisFile(this->cgi.scriptPath)) {
    this->isCGI = false;
    return;
  }

  this->isCGI = true;
}

const bool &Request::getIsCGI() const { return this->isCGI; }

bool Request::pathGCIisFile(std::string path) {
  struct stat buffer;

  if (stat(path.c_str(), &buffer) == -1) {
    return false;
  }

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
