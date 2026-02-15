#include "../includes/server.hpp"
#include <fstream>


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
    for (const auto& element : tokens) {
        std::cout << element << "\n";
    }
    
}
