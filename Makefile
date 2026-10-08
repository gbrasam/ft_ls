# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/10/02 11:41:25 by gebravo           #+#    #+#              #
#    Updated: 2026/10/08 14:15:37 by gebravo          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		= ft_ls

CC			= cc
CFLAGS		= -Wall -Wextra -Werror
DEPFLAGS	= -MMD -MP
INC			= -Iinclude

SRC_DIR		= src
OBJ_DIR		= obj
SRCS		= $(SRC_DIR)/main.c \
			  $(SRC_DIR)/init.c \
			  $(SRC_DIR)/parser.c \
			  $(SRC_DIR)/error.c \
			  $(SRC_DIR)/clean.c
OBJS		= $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS		= $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) $(INC) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean
	$(MAKE) all

-include $(DEPS)

.PHONY: all clean fclean re
