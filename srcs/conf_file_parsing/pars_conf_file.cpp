#include "../includes/server.hpp"
#include <fstream>
#include <algorithm>

void check_semicolon(std::vector<std::string>::iterator i){
    if (i->compare(";")){
        throw std::runtime_error("semicolon error");
    }
}

void location_parsing(std::vector<std::string> &tokens, std::vector<std::string>::iterator &i, ConfigFile &conf)
{
    location returned_loc;
    i++;
    returned_loc.path = *i;
    i++;
    if (i->compare("{"))
        throw std::runtime_error("error, no open curly brace");
    i++;
    while (i != tokens.end())
    {
        if (!i->compare("allow_methods"))
        {
            i++;
            if (!i->compare(";"))
                throw std::runtime_error("no methods in the location");
            while (i != tokens.end() && i->compare(";"))
            {
                if (i->compare("delete") && i->compare("post") && i->compare("get"))
                    throw std::runtime_error("not the best methods in the location");
                returned_loc.allow_methods.push_back(*i);
                i++;
            }
            check_semicolon(i);
        }
        else if (!i->compare("root"))
        {
            returned_loc.root = *(i + 1);
            i += 2;
            check_semicolon(i);
        }
        else if (!i->compare("index"))
        {
            returned_loc.index = *(i + 1);
            i += 2;
            check_semicolon(i);
        }
        else if (!i->compare("return"))
        {
            returned_loc.return_to = *(i + 1);
            i += 2;
            check_semicolon(i);
        }
        else if (!i->compare("autoindex"))
        {
            i++;
            if (i->compare("on") && i->compare("off"))
                throw std::runtime_error("error in the autoindex");
            else
            {
                if (!i->compare("on"))
                    returned_loc.autoindex = true;
                else
                    returned_loc.autoindex = false;
            }
            i++;
            check_semicolon(i);
        }
        else if (!i->compare("}"))
            break;
        else
            throw std::runtime_error("error, unknown element!");
        i++;
    }
    conf.locations.push_back(returned_loc);
}



void   get_config_server(std::vector<std::string> &tokens, ConfigFile &conf){
    int count = 0;
    int index = 0;
    std::vector<std::string>::iterator i = tokens.begin();
    while ((i = std::find(i, tokens.end(), "server")) != tokens.end())
    {
        count++;
        i++;
    }
    if (count > 1)
        throw std::runtime_error("Error: multiple servers in the config file");
    if (!tokens.begin()->compare("server") && !(tokens.begin() + 1)->compare("{") && !(tokens.end() - 1)->compare("}"))
    {
        for (i = tokens.begin(); i < tokens.end() - 1; i++){
            // std::cout << *i << std::endl;
            if(!i->compare("server"))
                i++;
            else if (!i->compare("listen")){
                i++;
                for(int j = 0; j < i->size(); j++){
                    if (!isdigit((*i)[j]))
                        throw std::runtime_error("listen is not a number");
                }
                int n = atoi(i->c_str());
                if ((std::find(conf.listen.begin(), conf.listen.end(), n) != conf.listen.end()) || n < 0 || n > 65535)
                    throw std::runtime_error("listen port duplicated!");
                if (index == 0)
                    index++;
                conf.listen.push_back(n);
                i++;
                check_semicolon(i);
            }
            else if (!i->compare("server_name"))
            {
                i++;
                conf.server_name = *i;
                i++;
                check_semicolon(i);
            }
            else if (!i->compare("root"))
            {
                i++;
                conf.root = *i;
                i++;
                check_semicolon(i);
            }
            // else if (!i->compare("location"))
            //     location_parsing(tokens, i, conf);
            else if (!i->compare("index"))
            {
                i++;
                conf.index = *i;
                i++;
                check_semicolon(i);
            }
            else if (!i->compare("client_max_body_size"))
            {
                i++;
                for(int j = 0; j < i->size(); j++){
                    if (!isdigit((*i)[j]))
                        throw std::runtime_error("client_max_body_size is not a number");
                }
                conf.client_max_size_body = atoi(i->c_str());
                i++;
                check_semicolon(i);
            }
            else if (!i->compare("host"))
            {
                i++;
                struct sockaddr_in sa;
                if (inet_pton(AF_INET, i->c_str(), &(sa.sin_addr)) != 1)
                    throw std::runtime_error("not an address");
                conf.host = *i;
                i++;
                check_semicolon(i);
            }
            else if (!i->compare("error_page"))
            {
                i++;
                for(int j = 0; j < i->size(); j++){
                    if (!isdigit((*i)[j]))
                        throw std::runtime_error("client_max_body_size is not a number");
                }
                conf.error_page[atoi(i->c_str())] = *(i + 1);
                i += 2;
                check_semicolon(i);
            }
            else if (!i->compare("cgi_conf"))
            {
                i++;
                if ((*i)[0] != '.')
                    throw std::runtime_error("cgi extension error");
                conf.cgi_config[*i] = *(i + 1);
                i += 2;
                check_semicolon(i);
            }
            else
            {
                throw std::runtime_error("config file error");
            }
        }
    }
    else
        throw std::runtime_error("config file error");
}


void ConfigFile::parse_config_file(char *av){
    std::ifstream inFile(av);
    std::string file;
    std::string line;
    size_t pos;
    std::vector<std::string> tokens;
    
    if (!inFile.is_open())
        throw std::runtime_error("can't open the config file");
    while(true){
        std::getline(inFile, line);
        if (line.empty() && inFile.eof())
            break;
        if ((pos = line.find("#", 0)) == std::string::npos){
            // std::cout << line << std::endl;
            file.append(line);
            line.clear();
        }
    }
    int start, end = 0;
    while (end < file.size())
    {
        while (end < file.size() && isspace(file[end]))
            end++;
        start = end;
        while(end < file.size() && !isspace(file[end]) && file[end] != '{' && file[end] != '}' && file[end] != ';')
            end++;
        if (file[end] == '{' || file[end] == '}' || file[end] == ';')
        {
            if (end != 0 && !isspace(file[end - 1]) && file[end - 1] != '{' && file[end - 1] != '{' && file[end - 1] != ';')
                tokens.push_back(file.substr(start, end - start));
            if (file[end] == '{')
                tokens.push_back("{");
            else if (file[end] == '}')
                tokens.push_back("}");
            else if (file[end] == ';')
                tokens.push_back(";");
            end++;
        }
        else
            tokens.push_back(file.substr(start, end - start));
    }
    // for (const auto& element : tokens) {
    //     std::cout << element << "\n";
    // }
    get_config_server(tokens, *this);
}
