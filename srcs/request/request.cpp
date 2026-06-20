#include "./../includes/request.hpp"

Request::Request(){}

void Request::setRequest(const std::string &req){
    std::cout << "hello from request and " << req << std::endl;
}