#include "../includes/Cgi.hpp"
#include "../includes/HttpResponse.hpp"
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>

// Looks up the interpreter configured for a script's file extension.
std::string Cgi::findInterpreter(const std::string &scriptPath,
                                  const std::map<std::string, std::string> &cgiConfig)
{
    size_t dotPos = scriptPath.find_last_of('.');
    if (dotPos == std::string::npos)
        throw 500;

    std::string extension = scriptPath.substr(dotPos);

    std::map<std::string, std::string>::const_iterator it = cgiConfig.find(extension);
    if (it == cgiConfig.end())
        throw 500;
    return it->second;
}

// Builds the CGI meta-variables and HTTP_* header variables as "KEY=VALUE" strings.
std::vector<std::string> Cgi::buildEnvp(Request &req)
{
    std::vector<std::string>envp;

    envp.push_back("REQUEST_METHOD=" + req.cgi.method);

    std::string scriptName = req.cgi.scriptPath.substr(req.conf.root.size());
    envp.push_back("SCRIPT_NAME=" + scriptName);
    envp.push_back("SCRIPT_FILENAME=" + req.cgi.scriptPath);

    envp.push_back("PATH_INFO=" + req.cgi.pathInfo);
    envp.push_back("QUERY_STRING=" + req.cgi.query);
    envp.push_back("CONTENT_TYPE=" + req.cgi.contentType);

    std::ostringstream lenStream;
    lenStream << req.cgi.contentLength;
    envp.push_back("CONTENT_LENGTH=" + lenStream.str());

    envp.push_back("SERVER_NAME=" + req.cgi.host);
    envp.push_back("SERVER_PORT=" + req.cgi.port);
    envp.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envp.push_back("GATEWAY_INTERFACE=CGI/1.1");

    static const std::string skipKeys[] = {
        "Content-Type", "Content-Length",
        "post-body", "filename", "binary-data"
    };
    static const size_t skipKeysCount = sizeof(skipKeys) / sizeof(skipKeys[0]);

    for (std::map<std::string, std::string>::const_iterator it = req.cgi.headers.begin();
         it != req.cgi.headers.end(); ++it)
    {
        bool skip = false;
        for (size_t i = 0; i < skipKeysCount; i++)
        {
            if (it->first == skipKeys[i])
            {
                skip = true;
                break;
            }
        }
        if (skip)
            continue;

        std::string key = "HTTP_" + it->first;
        for (size_t i = 0; i < key.size(); i++)
        {
            if (key[i] == '-')
                key[i] = '_';
            else
                key[i] = std::toupper(key[i]);
        }
        envp.push_back(key + "=" + it->second);
    }

    return envp;
}

// Forks, wires up the two pipes, chdir's into the script's directory, and execve's the interpreter.
CgiProcess Cgi::spawnProcess(const std::string &interpreter,
                                const std::string &scriptPath,
                                const std::string &workDir,
                                const std::vector<std::string> &env)
{
    int inPipe[2];
    int outPipe[2];

    if (pipe(inPipe) == -1)
        throw 500;
    if (pipe(outPipe) == -1)
    {
        close(inPipe[0]);
        close(inPipe[1]);
        throw 500;
    }

    size_t slash = scriptPath.find_last_of('/');
    std::string scriptBase = (slash == std::string::npos) ? scriptPath : scriptPath.substr(slash + 1);

    char *argv[3];
    argv[0] = const_cast<char *>(interpreter.c_str());
    argv[1] = const_cast<char *>(scriptBase.c_str());
    argv[2] = NULL;

    std::vector<char *> rawEnv;
    for (size_t i = 0; i < env.size(); i++)
        rawEnv.push_back(const_cast<char *>(env[i].c_str()));
    rawEnv.push_back(NULL);

    pid_t pid = fork();
    if (pid == -1)
    {
        close(inPipe[0]);
        close(inPipe[1]);
        close(outPipe[0]);
        close(outPipe[1]);
        throw 500;
    }

    if (pid == 0)
    {
        close(inPipe[1]);
        close(outPipe[0]);

        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        close(inPipe[0]);
        close(outPipe[1]);

        if (chdir(workDir.c_str()) == -1)
            _exit(1);

        execve(interpreter.c_str(), argv, &rawEnv[0]);
        _exit(1);
    }

    close(inPipe[0]);
    close(outPipe[1]);

    fcntl(inPipe[1], F_SETFL, O_NONBLOCK);
    fcntl(outPipe[0], F_SETFL, O_NONBLOCK);

    CgiProcess proc;
    proc.pid = pid;
    proc.stdinFd = inPipe[1];
    proc.stdoutFd = outPipe[0];
    proc.bodyWritten = 0;
    proc.startTime = time(NULL);
    return proc;
}

// Entry point, validates the extension, builds the environment, spawns the process, and seeds it with the request body.
CgiProcess Cgi::start(Request &req)
{
    std::string interpreter = findInterpreter(req.cgi.scriptPath, req.conf.cgi_config);
    std::vector<std::string> envp = buildEnvp(req);

    size_t slash = req.cgi.scriptPath.find_last_of('/');
    std::string workDir = (slash == std::string::npos) ? "." : req.cgi.scriptPath.substr(0, slash);

    CgiProcess proc = spawnProcess(interpreter, req.cgi.scriptPath, workDir, envp);

    proc.body = req.cgi.body;
    if (proc.body.empty())
    {
        close(proc.stdinFd);
        proc.stdinFd = -1;
    }

    return proc;
}

// Splits a finished CGI script's raw stdout into headers and body for an HttpResponse.
HttpResponse Cgi::parseOutput(const std::string &output) //remove this 
{
    HttpResponse response;
    response.setStatusCode(200);

    size_t headerEnd = output.find("\r\n\r\n");
    size_t sepLen = 4;
    if (headerEnd == std::string::npos)
    {
        headerEnd = output.find("\n\n");
        sepLen = 2;
    }
    if (headerEnd == std::string::npos)
    {
        response.setBody(output);
        response.addHeader("Content-Type", "text/plain");
        return response;
    }

    std::string headerBlock = output.substr(0, headerEnd);
    std::string body = output.substr(headerEnd + sepLen);

    std::istringstream headerStream(headerBlock);
    std::string line;
    while (std::getline(headerStream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value[0] == ' ')
            value.erase(0, 1);

        if (key == "Status")
        {
            int code = std::atoi(value.c_str());
            if (code > 0)
                response.setStatusCode(code);
        }
        else if (key == "Content-Length")
        {
        }
        else
            response.addHeader(key, value);
    }

    response.setBody(body);
    return response;
}
