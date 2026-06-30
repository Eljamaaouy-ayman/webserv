CXX = c++
CXXFLAGS = # -Wall -Wextra -Werror -std=c++98
NAME = webserv

SRCS = 	./srcs/main.cpp \
		./srcs/conf_file_parsing/pars_conf_file.cpp \
		./srcs/server/Server.cpp \
		./srcs/server/Client.cpp \
<<<<<<< HEAD
		./srcs/request/request.cpp
=======
		./srcs/server/Request.cpp
>>>>>>> 1ee6db6fd43e2c7e1e2693b1460e8793032a6375


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
