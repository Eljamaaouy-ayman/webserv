CXX = c++
CXXFLAGS =  -Wall -Wextra -Werror -std=c++98
NAME = webserv

SRCS = 	./srcs/main.cpp \
		./srcs/conf_file_parsing/pars_conf_file.cpp


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
