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

