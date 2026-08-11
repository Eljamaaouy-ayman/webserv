#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <sys/types.h>
#include "request.hpp"

class HttpResponse;

struct CgiProcess
{
    pid_t pid;
    int stdinFd;
    int stdoutFd;
    std::string body;
    size_t bodyWritten;
    std::string output;
    time_t startTime;
};

class Cgi
{
public:
    static CgiProcess start(Request &req);

    static HttpResponse parseOutput(const std::string &output);

private:
    static std::string findInterpreter(const std::string &scriptPath,
                                        const std::map<std::string, std::string> &cgiConfig);

    static std::vector<std::string> buildEnvp(Request &req);

    static CgiProcess spawnProcess(const std::string &interpreter,
                                    const std::string &scriptPath,
                                    const std::string &workDir,
                                    const std::vector<std::string> &env);
};
