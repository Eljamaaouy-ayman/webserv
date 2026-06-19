#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP
#include <string>
#include <sstream>
#include <cstdlib>
#include <map>

class SessionManager
{
private:
    static std::map<std::string, std::string> sessions;
    static std::string generateSessionID();

public:
    static std::string createSession(const std::string &user);
    static void destroySession (const std::string &sessionID);
    static std::string getSessionData(const std::string &sessionID);
};

#endif