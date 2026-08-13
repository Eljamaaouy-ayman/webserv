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
};

#endif