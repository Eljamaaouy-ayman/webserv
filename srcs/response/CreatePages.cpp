#include "../includes/includes.hpp"

std::string CreatePages::AutoIndexPage(DIR *dir, const std::string &path, bool isRoot)
{
    std::ostringstream page;
    page << "<!DOCTYPE html>\n"
            "<html lang=\"en\">\n"
            "<head>\n"
            "<meta charset=\"UTF-8\">\n"
            "<title>Index of /</title></head>"
            "<body style=\"font-family: monospace; margin:40px;\">"
            "<h1 style=\"border-bottom:1px solid #ccc; padding-bottom:10px;\">Index of /</h1>\n"
            "<ul style=\"list-style:none; padding-left:0;\">\n";
    struct dirent *entry;
    if (!isRoot)
        page << "<li style=\"padding:6px 0;\"><a style=\"text-decoration:none; color:#0066cc;\" href=\"../\">../</a></li>\n";
    while ((entry = readdir(dir)) != NULL)
    {
        if (std::string(entry->d_name) == "." || (std::string(entry->d_name) == ".."))
            continue;
        std::string name = RequestHandler::isDirectory(path + '/' + entry->d_name) ? std::string(entry->d_name) + '/' : std::string(entry->d_name);
        page << "<li style=\"padding:6px 0;\"><a style=\"text-decoration:none; color:#0066cc;\" href=\"" + name + "\">" + name + "</a></li>\n";
    }
    page << "</ul>\n"
            "</body>\n"
            "</html>\n";
    return page.str();
}

std::string CreatePages::LoginPage(const std::string &errorMsg)
{
    std::ostringstream page;
    page << "<!DOCTYPE html>\n"
            "<html>\n"
            "<head><title>Login</title></head>\n"
            "<body>\n";
    if (errorMsg.empty() == false)
        page << "<h3 style=\"color: red;\">\n"
             << errorMsg << "</h3>\n";
    page << "<h2>Login</h2>\n"
            "<form method=POST action=/login>\n"
            "Username:<br>\n"
            "<input type=text name=username><br><br>\n"
            "Password:<br>\n"
            "<input type=password name=password><br><br>\n"
            "<input type=submit value=Login>\n"
            "</form>\n"
            "</body>\n"
            "</html>\n";
    return page.str();
}
std::string CreatePages::RegisterPage(const std::string &errorMsg)
{
    std::ostringstream page;
    page << "<!DOCTYPE html>\n"
            "<html>\n"
            "<head><title>Register</title></head>\n"
            "<body>\n";
    if (errorMsg.empty() == false)
        page << "<h3 style=\"color: red;\">\n"
             << errorMsg << "</h3>\n";
    page << "<h2>Register</h2>\n"
            "<form method=POST action=/register>\n"
            "Username:<br>\n"
            "<input type=text name=username><br><br>\n"
            "Password:<br>\n"
            "<input type=password name=password><br><br>\n"
            "<input type=submit value=Register>\n"
            "</form>\n"
            "</body>\n"
            "</html>\n";
    return page.str();
}
