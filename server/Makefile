NAME = zappy

#########
RM = rm -rf
CC = cc
CFLAGS = -Werror -Wextra -Wall -g -O3
LDFLAGS = -lm
RELEASE_CFLAGS = -Werror -Wextra -Wall -g -O3
#########

#########
FILES = main parse_arg ssl_al

SRC = $(addsuffix .c, $(FILES))

vpath %.c srcs srcs/parse_arg srcs/server
#########

#########
OBJ_DIR = objs
OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
DEP = $(addsuffix .d, $(basename $(OBJ)))
#########

#########
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	${CC} -MMD $(CFLAGS) -Isrcs/parse_arg -Isrcs/server -c $< -o $@

all: .gitignore certs
	$(MAKE) $(NAME)

$(NAME): $(OBJ) Makefile
	$(CC) $(CFLAGS) $(OBJ) -lssl -lcrypto -o $(NAME) $(LDFLAGS)
	@echo "EVERYTHING DONE  "
#	@./.add_path.sh

release: CFLAGS = $(RELEASE_CFLAGS)
release: re
	@echo "RELEASE BUILD DONE  "

clean:
	$(RM) $(OBJ) $(DEP)
	$(RM) -r $(OBJ_DIR)
	@echo "OBJECTS REMOVED   "

fclean: clean
	$(RM) $(NAME)
	$(RM) -r certs
	@echo "EVERYTHING REMOVED   "

re: fclean
	$(MAKE) all CFLAGS="$(CFLAGS)"

.gitignore:
	@if [ ! -f .gitignore ]; then \
		echo ".gitignore not found, creating it..."; \
		echo ".gitignore" >> .gitignore; \
		echo "$(NAME)" >> .gitignore; \
		echo "$(OBJ_DIR)/" >> .gitignore; \
		echo "*.pem" >> .gitignore; \
		echo ".gitignore created and updated with entries."; \
	else \
		echo ".gitignore already exists."; \
	fi

certs:
	@if [ ! -f certs/cert.pem ]; then \
		echo "Certificates not found, creating them..."; \
		mkdir -p certs; \
		openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout certs/key.pem -out certs/cert.pem -subj "/CN=localhost"; \
		echo "Certificates created."; \
	else \
		echo "Certificates already exist."; \
	fi

.PHONY: all clean fclean re release .gitignore certs

-include $(DEP)