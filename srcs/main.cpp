#include "includes/server.hpp"

int main(int ac, char ** av){
    Request request;
    try{
        if (ac == 1){
            char file[] = "config/default.conf";
            request.conf.parse_config_file(file);
        }
        else if (ac == 2)
            request.conf.parse_config_file(av[1]);
        else
            throw std::runtime_error("./program configFile");
        
    } catch (const std::exception &e){
        std::cerr << e.what() << '\n';
        return 1;
    }
}