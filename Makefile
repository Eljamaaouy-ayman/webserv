CXX = c++
CXXFLAGS = # -Wall -Wextra -Werror -std=c++98
NAME = webserv

SRCS = 	./srcs/main.cpp \
		./srcs/conf_file_parsing/pars_conf_file.cpp \
		./srcs/server/Server.cpp \
		./srcs/server/Client.cpp \
		./srcs/cgi/Cgi.cpp \
		./srcs/request/request.cpp \
		./srcs/response/CreatePages.cpp \
		./srcs/response/HttpResponse.cpp \
		./srcs/response/RequestHandler.cpp \
		./srcs/response/SessionManager.cpp \
		./srcs/response/UserManager.cpp \
		./srcs/response/Utils.cpp \



OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
		$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean: 
	rm -rf $(OBJS)

fclean: clean
		rm -rf $(NAME)

re: fclean all

.SECONDARY: $(OBJS)