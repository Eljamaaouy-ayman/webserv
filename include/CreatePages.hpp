#ifndef CREATEPAGES_HPP
#define CREATEPAGES_HPP
#include <string>
#include <sstream>
#include <dirent.h>

class CreatePages
{
public:

    static std::string UploadsListPage();
    static std::string LoginPage(const std::string &errorMsg);
    static std::string RegisterPage(const std::string &errorMsg);
    static std::string UploadsPage();
};

#endif