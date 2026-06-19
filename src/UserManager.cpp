#include "../include/UserManager.hpp"

std::map<std::string, std::string> UserManager::users;

bool UserManager::addUser(const std::string &username, const std::string &password)
{
    if (users.find(username) != users.end())
        return false;
    users[username] = password;
    return true;
}
bool UserManager::authenticateUser(const std::string &username, const std::string &password)
{
    if (users.find(username) == users.end())
        return false;
    return (users[username] == password);
}
