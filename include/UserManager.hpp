#ifndef USERMANAGER_HPP
#define USERMANAGER_HPP
#include <map>
#include <string>

class UserManager
{
private:
    static std::map<std::string, std::string> users;

public:
    static bool addUser(const std::string &username, const std::string &password);
    static bool authenticateUser(const std::string &username, const std::string &password);
};

#endif