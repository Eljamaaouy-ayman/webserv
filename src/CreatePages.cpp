#include "../include/CreatePages.hpp"

std::string CreatePages::AutoIndexPage(DIR *dir, const std::string &path, bool isRoot)
{
    std::ostringstream page;
    page << "<!DOCTYPE html>\n"
            "<html lang = \"en\">\n"
            "<head>\n"
            "<meta charset = \"UTF-8\">\n"
            "<title> Index of / </ title></ head><body style = \"font-family: monospace; margin:40px;\"><h1 style = \"border-bottom:1px solid #ccc; padding-bottom:10px;\"> Index of / </ h1>\n"
            "<ul style = \"list-style:none; padding-left:0;\">\n";

    struct dirent *entry;
    if (!isRoot)
        page << "<li style=\"padding:6px 0;\"><a style=\"text-decoration:none; color:#0066cc;\" href=\"../\">../</a></li>\n";
    while ((entry = readdir(dir)) != NULL)
    {
        if (std::string(entry->d_name) == "." || (std::string(entry->d_name) == ".."))
            continue;
        std::string name = RequestHandler::isDirectory(path + '/' + entry->d_name) ? entry->d_name + '/' : entry->d_name;
        page << "<li style=\"padding:6px 0;\"><a style=\"text-decoration:none; color:#0066cc;\" href=\"" + name + "\">" + name + "</a></li>\n";
    }
    page << "</ul>\n"
            "</body>\n"
            "</html>\n";
    return page.str();
}

std::string CreatePages::UploadsListPage()
{
    std::ostringstream page;
    page << "<!DOCTYPE html>"
            "<html>\n"
            "<head>\n"
            "    <title>Uploads</title>\n"
            "    <script src=\"https://cdn.tailwindcss.com\"></script>\n"
            "</head>"
            "<body class=\"bg-gradient-to-br from-slate-900 to-slate-800 min-h-screen flex items-center justify-center\">\n"
            "<div class=\"bg-slate-950/70 backdrop-blur-lg p-8 rounded-2xl shadow-2xl w-[500px] border border-slate-700\">\n"
            "<h2 class=\"text-2xl font-bold text-white mb-6 text-center tracking-wide\">Uploaded Files</h2>\n"
            "<ul id=\"fileList\" class=\"space-y-4\">\n";

    DIR *dir = opendir("www/uploads");

    struct dirent *file;
    if (dir)
    {
        while ((file = readdir(dir)) != NULL)
        {
            std::string filename = file->d_name;
            if (filename == "." || filename == "..")
                continue;
            page << "<li class=\"flex items-center justify-between bg-slate-800 hover:bg-slate-700 transition p-4 rounded-xl shadow-lg\">\n"
                    "<a href=\"/uploads/"
                 << filename << "\" class=\"text-blue-400 hover:text-cyan-300 font-medium\">" << filename << "</a>\n"
                                                                                                             "<button class=\"bg-red-500 hover:bg-red-600 text-white px-4 py-1.5 rounded-lg shadow-md\"\n"
                                                                                                             " onclick=\"deleteFile('"
                 << filename << "', this)\">Delete</button>\n"
                                "</li>\n";
        }
    }
    page << "</ul>\n"
            "</div>\n"
            "<script>\n"
            "function deleteFile(filename, btn) {\n"
            "    fetch('/uploads/' + filename, { method:'DELETE' })\n"
            "        .then(r => r.ok ? btn.parentElement.remove() : alert('Failed to delete ' + filename))\n"
            "        .catch(e => alert('Error: ' + e));\n"
            "}\n"
            "</script>\n"
            "</body>\n"
            "</html>\n";
    closedir(dir);
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
std::string CreatePages::UploadsPage()
{
    std::ostringstream page;
    page << "<!DOCTYPE html>"
            "<html>\n"
            "<head>\n"
            "    <title>Uploads</title>\n"
            "    <script src=\"https://cdn.tailwindcss.com\"></script>\n"
            "</head>"
            "<body class=\"bg-gradient-to-br from-slate-900 to-slate-800 min-h-screen flex items-center justify-center\">\n"
            "<div class=\"bg-slate-950/70 backdrop-blur-lg p-8 rounded-2xl shadow-2xl w-[500px] border border-slate-700\">\n"
            "<h2 class=\"text-2xl font-bold text-white mb-6 text-center tracking-wide\">Uploaded Files</h2>\n"
            "<ul id=\"fileList\" class=\"space-y-4\">\n";

    DIR *dir = opendir("www/uploads");

    struct dirent *file;
    if (dir)
    {
        while ((file = readdir(dir)) != NULL)
        {
            std::string filename = file->d_name;
            if (filename == "." || filename == "..")
                continue;
            page << "<li class=\"flex items-center justify-between bg-slate-800 hover:bg-slate-700 transition p-4 rounded-xl shadow-lg\">\n"
                    "<a href=\"/uploads/"
                 << filename << "\" class=\"text-blue-400 hover:text-cyan-300 font-medium\">" << filename << "</a>\n"
                                                                                                             "<button class=\"bg-red-500 hover:bg-red-600 text-white px-4 py-1.5 rounded-lg shadow-md\"\n"
                                                                                                             " onclick=\"deleteFile('"
                 << filename << "', this)\">Delete</button>\n"
                                "</li>\n";
        }
        closedir(dir);
    }
    page << "</ul>\n"
            "</div>\n"
            "<script>\n"
            "function deleteFile(filename, btn) {\n"
            "    fetch('/uploads/' + filename, { method:'DELETE' })\n"
            "        .then(r => r.ok ? btn.parentElement.remove() : alert('Failed to delete ' + filename))\n"
            "        .catch(e => alert('Error: ' + e));\n"
            "}\n"
            "</script>\n"
            "</body>\n"
            "</html>\n";

    return page.str();
}