#include "../includes/request.hpp"

Request::Request() : method("ELSE"), path(""), httpV(""), isCGI(false)
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

// void Request::setRequest(const std::string &req)
// {
//   if (!this->request.empty()) {
//     this->request.clear();
//   }

//   // check if the (path, httpV) is not empty if not we need to clear
//   // the old data
//   if (!this->path.empty()) {
//     this->path.clear();
//   }
//   if (!this->httpV.empty()) {
//     this->httpV.clear();
//   }

//   // check if the cgi is not empty if not we need to clear the old data
//   if (!this->cgi.host.empty()) {
//     this->cgi.host.clear();
//   }
//   if (!this->cgi.port.empty()) {
//     this->cgi.port.clear();
//   }
//   if (!this->cgi.method.empty()) {
//     this->cgi.method.clear();
//   }
//   if (!this->cgi.scriptPath.empty()) {
//     this->cgi.scriptPath.clear();
//   }
//   if (!this->cgi.pathInfo.empty()) {
//     this->cgi.pathInfo.clear();
//   }
//   if (!this->cgi.query.empty()) {
//     this->cgi.query.clear();
//   }
//   if (!this->cgi.headers.empty()) {
//     this->cgi.headers.clear();
//   }
//   if (!this->cgi.body.empty()) {
//     this->cgi.body.clear();
//   }
//   if (this->cgi.contentLength != 0) {
//     this->cgi.contentLength = 0;
//   }
//   if (!this->cgi.contentType.empty()) {
//     this->cgi.contentType.clear();
//   }

//   bool iFoundCookie = false;
//   std::stringstream ss(req);

//   // get method & path & http version
//   std::string line;
//   if (std::getline(ss, line)) {
//     std::stringstream firstLine(line);
//     std::string reqMethod;
//     firstLine >> reqMethod;
//     if (reqMethod == "GET") {
//       this->method = "GET";
//     } else if (reqMethod == "POST") {
//       this->method = "POST";
//     } else if (reqMethod == "DELETE") {
//       this->method = "DELETE";
//     } else {
//       this->method = "ERROR";
//     }

//     firstLine >> this->path >> this->httpV;

//     // check http version
//     if (this->httpV != "HTTP/1.0" && this->httpV != "HTTP/1.1") {
//       this->method = "ERROR"; // return BAD_REQUEST
//     }

//     // check if request is CGI
//     this->checkCGI(this->path);
//   }

//   // get the headers
//   std::string key, value;
//   size_t pos;
//   while (std::getline(ss, line)) {
//     pos = line.find(":");

//     if (pos == std::string::npos) {
//       if (line != "\r") {
//         this->method = "ERROR";
//       }
//       break;
//     }

//     key = line.substr(0, pos);
//     value = line.substr(pos + 2, line.length());

//     // check if key or value is empty
//     if (key.empty() || value.empty()) {
//       this->method = "ERROR"; // return BAD_REQUEST
//     }

//     this->request[key] = value;
//   }

//   // check if we have host header
//   std::map<std::string, std::string>::iterator itHost =
//       this->request.find("Host");
//   if (itHost != this->request.end()) {
//     // check host and port
//     // check host
//     size_t posHost;

//     std::string hostValue = itHost->second;
//     posHost = hostValue.find(":");
//     if (posHost != std::string::npos) {
//       std::string domainName = hostValue.substr(0, posHost);
//       if (domainName != "localhost") {
//         if (domainName != this->conf.host) {
//           this->method = "ERROR"; // return BAD_REQUEST
//         }
//       }

//       int reqListen = std::atoi(hostValue.substr(posHost + 1).c_str());
//       std::vector<int>::iterator it = std::find(
//           this->conf.listen.begin(), this->conf.listen.end(), reqListen);
//       if (it == this->conf.listen.end()) {
//         this->method = "ERROR"; // return BAD_REQUEST
//       }
//     } else {
//       this->method = "ERROR"; // return BAD_REQUEST
//     }
//   } else {
//     this->method = "ERROR"; // return BAD_REQUEST
//   }

//   // check the Content-Length in method post
//   if (this->method == "POST") {
//     std::map<std::string, std::string>::iterator itContentLength =
//         this->request.find("Content-Length");
//     if (itContentLength != this->request.end()) {
//       int contentLengthValue = std::atoi(itContentLength->second.c_str());
//       if (contentLengthValue < 0) {
//         this->method = "ERROR"; // return BAD_REQUEST
//       }

//     } else {
//       this->method = "ERROR"; // return BAD_REQUEST
//     }
//   }

//   // check if client send cookies
//   std::map<std::string, std::string>::iterator itCookie =
//       this->request.find("Cookie");
//   if (itCookie != this->request.end()) {
//     iFoundCookie = true;
//   }

//   // get the body
//   pos = req.find("\r\n\r\n");
//   if (pos == std::string::npos) {
//     return;
//   }

//   value = req.substr(pos + 4, req.length());

//   if (!value.empty()) {
//     key = "post-body";
//     this->request[key] = value;
//   }

//   // Parse Body

//   std::string contentType =
//       this->request.count("Content-Type") ? this->request["Content-Type"] : "";
//   if (contentType.substr(0, 52) ==
//       "multipart/form-data; boundary=----WebKitFormBoundary") {

//     // check info of post-body (get file name)
//     // get file name
//     pos = this->request["post-body"].find("Content-Disposition: ");
//     // * check if not found Content-Disposition
//     if (pos == std::string::npos) {
//       std::cerr << "Content-Disposition not found" << std::endl;
//       return;
//     }

//     pos = this->request["post-body"].find("filename=\"", pos);
//     while ((pos = this->request["post-body"].find("filename=\"", pos)) !=
//            std::string::npos) {
//       if (this->request["post-body"][pos - 1] != '"') {
//         break;
//       }
//       pos++;
//     }

//     if (pos == std::string::npos) {
//       std::cerr << "filename not found" << std::endl;
//       return;
//     }

//     size_t start = pos + 10;
//     size_t end = this->request["post-body"].find("\"", start);
//     key = "filename";
//     value = this->request["post-body"].substr(start, end - start);
//     this->request[key] = value;

//     // get binary data of file
//     pos = this->request["post-body"].find("\r\n\r\n", end);
//     if (pos == std::string::npos) {
//       std::cerr << "binary data of file not found" << std::endl;
//       return;
//     }

//     // find last boundary
//     end = this->request["post-body"].rfind("\r\n------WebKitFormBoundary");
//     if (end == std::string::npos) {
//       std::cerr << "last boundary not found" << std::endl;
//       return;
//     }

//     key = "binary-data";
//     start = pos + 4;
//     value = this->request["post-body"].substr(start, end - start);

//     if (!value.empty()) {
//       this->request[key] = value;
//     } else {
//       // handle the empty file when you upload a non empty file and after that
//       // you remove all data and get it again
//       this->request.erase(key);
//     }
//   }

//   // remove Cookies from request (because when the user remove it from browser
//   // it still store it in request map)
//   if (!iFoundCookie) {
//     this->request.erase("Cookie");
//   }

//   if (this->getIsCGI()) {
//     // copy host of request to cgi host
//     this->cgi.host = this->request.count("Host")
//                          ? request.find("Host")->second.substr(
//                                0, this->request.find("Host")->second.find(":"))
//                          : "";
//     // copy listen of request to cgi listen
//     this->cgi.port = this->request.count("Host")
//                          ? request.find("Host")->second.substr(
//                                this->request.find("Host")->second.find(":") + 1)
//                          : "";
//     // copy method of request to cgi method
//     this->cgi.method = this->getMethodByName(this->method);
//     // copy headers of request to cgi headers
//     this->cgi.headers = this->request;
//     // copy body of request to cgi body
//     this->cgi.body = this->request.count("post-body")
//                          ? this->request.find("post-body")->second
//                          : "";
//     // add Content Length
//     this->cgi.contentLength =
//         this->request.count("Content-Length")
//             ? std::strtol(this->request.find("Content-Length")->second.c_str(),
//                           NULL, 10)
//             : 0;
//     // copy content type of request to cgi content type
//     this->cgi.contentType = this->request.count("Content-Type")
//                                 ? this->request.find("Content-Type")->second
//                                 : "";
//   }
// }


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
