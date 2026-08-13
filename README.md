*This project has been created as part of the 42 curriculum by \<ynadime>, \<aben-hzz>, \<ael-jama>.*

# Webserv

## Description

**Webserv** is a project from the 42 curriculum focused on implementing a functional **HTTP/1.1 web server in C++98**.

The main goal of the project is to understand how web servers work internally by implementing the different components ourselves, rather than relying on an existing web server.

The server is responsible for accepting client connections, receiving and parsing HTTP requests, processing them according to a configuration file, generating HTTP responses, and sending those responses back to clients.

The project covers several important concepts in networking and systems programming, including:

- TCP/IP socket programming
- HTTP/1.1
- Non-blocking I/O
- I/O multiplexing
- HTTP request parsing
- HTTP response generation
- Configuration file parsing
- Static file serving
- HTTP methods such as `GET`, `POST`, and `DELETE`
- File uploads using `multipart/form-data`
- CGI execution
- Error handling
- Multiple simultaneous client connections

The general architecture of the server can be summarized as:

                         ┌──────────────┐
                         │    Client    │
                         └──────┬───────┘
                                │
                           HTTP Request
                                │
                                ▼
                         ┌──────────────┐
                         │    Socket    │
                         └──────┬───────┘
                                │
                                ▼
                         ┌──────────────┐
                         │ Multiplexer  │
                         └──────┬───────┘
                                │
                                ▼
                         ┌──────────────┐
                         │Request Parser│
                         └──────┬───────┘
                                │
                                ▼
                       ┌──────────────────┐
                       │ Route / Config   │
                       │    Matching      │
                       └────────┬─────────┘
                                │
                  ┌─────────────┼─────────────┐
                  │             │             │
                  ▼             ▼             ▼
              Static File      CGI        Upload /
                                           DELETE
                  │             │             │
                  └─────────────┼─────────────┘
                                │
                                ▼
                         ┌──────────────┐
                         │   Response   │
                         │   Generator  │
                         └──────┬───────┘
                                │
                                ▼
                         ┌──────────────┐
                         │    Socket    │
                         └──────┬───────┘
                                │
                         HTTP Response
                                │
                                ▼
                         ┌──────────────┐
                         │    Client    │
                         └──────────────┘

## Instructions


The project requires:

A Unix-like operating system
A C++ compiler with C++98 support
make

The project must be compiled using the C++98 standard.

Compilation

Clone the repository:

git clone https://github.com/Eljamaaouy-ayman/webserv
cd webserv

Compile the project:

make

To remove object files:

make clean

To remove object files and the executable:

make fclean

To recompile the project from scratch:

make re
Execution

Run the server by providing a configuration file:

./webserv <configuration_file>

For example:

./webserv config/default.conf

Once the server is running, it can be accessed through a web browser or an HTTP client such as curl.

For example:

curl http://localhost:8080/
Testing
GET
curl -v http://localhost:8080/
POST
curl -v -X POST http://localhost:8080/test \
     -d "name=webserv"
DELETE
curl -v -X DELETE http://localhost:8080/test.txt

To inspect only the HTTP response headers:

curl -I http://localhost:8080/
Configuration

The server is configured through a configuration file.

A configuration can specify information such as:

Listening port
Server address
Server name
Root directory
Allowed HTTP methods
Index files
CGI extensions
Upload directory
Client body size
Error pages
Redirects
Location-specific settings

Example:

server {
    listen 8080;
    server_name localhost;

    root ./www;

    client_max_body_size 10M;

    location / {
        allowed_methods GET;
        index index.html;
    }

    location /upload {
        allowed_methods POST;
        upload_store ./uploads;
    }

    location /cgi-bin {
        allowed_methods GET POST;
        cgi .py /usr/bin/python3;
    }
}

The exact syntax depends on the configuration parser implemented in the project.

---

## License

This project was developed as part of the 42 Common Core curriculum at 1337 School for educational purposes.

---


## Resources

- [HTTP/1.0 Specification](https://datatracker.ietf.org/doc/html/rfc1945)
- [HTTP MDN Docs](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- [CGI Specification](https://tools.ietf.org/html/rfc3875)
- [C++ Reference](https://cppreference.com/)
- [Create a simple HTTP server in c](https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa)
- [IBM - Non-Blocking I/O & select](https://www.ibm.com/docs/en/i/7.2.0?topic=designs-example-nonblocking-io-select)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/index-wide.html)
- [Socket Programming in c](https://www.geeksforgeeks.org/c/socket-programming-cc/)
- [All about sockets blocking](http://dwise1.net/pgm/sockets/blocking.html)
- [epoll man page](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [How does epoll really work?](https://copyconstruct.medium.com/the-method-to-epolls-madness-d9d2d6378642)
- [The C10K Problem](http://www.kegel.com/c10k.html)
- [Nginx Documentation](https://nginx.org/en/docs/)


## AI Usage

AI tools were used throughout the development of the project as a support tool for learning, debugging, and documentation.

AI was used for:

- Understanding the overall architecture and requirements of the Webserv project.
- Researching and clarifying networking and socket programming concepts.
- Understanding client/server communication and TCP connections.
- Learning about non-blocking sockets and I/O multiplexing.
- Understanding and debugging the server's event loop.
- Getting explanations of C++98 concepts, syntax, and standard library functions.
- Helping identify and debug compilation errors, runtime errors, and unexpected behavior.
- Discussing possible implementations and comparing different approaches to solving technical problems.
- Understanding process management and communication when implementing CGI.
- Investigating HTTP concepts and how they relate to the implementation.
- Testing and reasoning about edge cases and possible server behaviors.
- Improving code organization and readability.
- Reviewing parts of the implementation and identifying potential issues.
- Helping write and structure project documentation, including this README.

AI was used as a learning and development assistant. All code and suggestions were reviewed, adapted, and tested as part of the project, with the final implementation being understood and maintained by the project authors.

