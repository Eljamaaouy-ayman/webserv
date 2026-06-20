#include "../include/SessionManager.hpp"

std::map<std::string, std::string> SessionManager::sessions;

std::string SessionManager::generateSessionID()
{
    std::ostringstream oss;
    do
    {
        oss.clear();
        oss.str("");
        for (int i = 0; i < 32; i++)
            oss << "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 62];
    } while (sessions.find(oss.str()) != sessions.end());

    return oss.str();
}

std::string SessionManager::createSession(const std::string &user)
{
    std::string sessionID = generateSessionID();
    sessions[sessionID] = user;
    return sessionID;
}

void SessionManager::destroySession(const std::string &sessionID)
{
    sessions.erase(sessionID);
}
std::string SessionManager::getSessionData(const std::string &sessionID)
{
    if (sessions.find(sessionID) == sessions.end())
        return "";
    return sessions[sessionID];
}

std::string SessionManager::extractSessionID(const std::string &cookie)
{
    size_t pos = cookie.find("session_id=");
    if (pos == std::string::npos || pos + 11 >= cookie.size())
        return "";
    size_t start = pos + 11;
    if (start >= cookie.size())
        return "";
    size_t end = cookie.find(';', start);
    std::string sessionID = end == std::string::npos ? cookie.substr(start) : cookie.substr(start, end - start);
    std::string ws = " \t\n\r\f\v";
    start = sessionID.find_first_not_of(ws);
    end = sessionID.find_last_not_of(ws);
    if (start == std::string::npos || end == std::string::npos)
        return "";
    sessionID = sessionID.substr(start, end - start + 1);
    if (sessionID.find_first_of(ws + ";") != std::string::npos)
        return "";
    return sessionID;
}
