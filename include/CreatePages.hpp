#ifndef CREATEPAGES_HPP
#define CREATEPAGES_HPP
#include <string>
#include <sstream>
#include <dirent.h>
#include "RequestHandler.hpp"

class CreatePages
{
public:
    static std::string AutoIndexPage(DIR *dir, const std::string &path, bool isRoot);
    static std::string UploadsListPage();
    static std::string LoginPage(const std::string &errorMsg);
    static std::string RegisterPage(const std::string &errorMsg);
    static std::string UploadsPage();
};

#endif