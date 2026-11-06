NAME=3dmotor
CC=gcc
CFLAGS=-Wall -Wextra -Werror -g -Iinclude -lm

SRC_DIR=src
OBJ_DIR=build

SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS) )

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $^ -o $@
	
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run :
	./$(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
