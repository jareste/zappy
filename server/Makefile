NAME = zappy

#########
RM = rm -rf
CC = cc
CFLAGS = -Werror -Wextra -Wall -g -O3 -I$(OPENSSL_BUILD_DIR)/include
LDFLAGS = -L$(OPENSSL_BUILD_DIR)/lib -lssl -lcrypto -lm
RELEASE_CFLAGS = -Werror -Wextra -Wall -g -O3
#########

#########
FILES = main parse_arg ssl_al ssl_table

SRC = $(addsuffix .c, $(FILES))

vpath %.c srcs srcs/parse_arg srcs/server
#########

#########
THIRD_PARTY_PATH = third_party

OPENSSL_SRC_DIR := third_party/openssl
OPENSSL_INSTALL_DIR := third_party/openssl_install

OPENSSL_BUILD_DIR := third_party/openssl_build
OPENSSL_BUILD_DONE := $(OPENSSL_INSTALL_DIR)/lib/libssl.a
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

all: .gitignore create_cert
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
	cd $(OPENSSL_SRC_DIR) && [ -f Makefile ] && make clean || true
	rm -rf $(OPENSSL_INSTALL_DIR)
	@echo "EVERYTHING REMOVED   "

re: fclean
	$(MAKE) all CFLAGS="$(CFLAGS)"

.gitignore:
	@if [ ! -f .gitignore ]; then \
		echo ".gitignore not found, creating it..."; \
		echo ".gitignore" >> .gitignore; \
		echo "$(NAME)" >> .gitignore; \
		echo "$(OBJ_DIR)/" >> .gitignore; \
		echo ".gitignore created and updated with entries."; \
	else \
		echo ".gitignore already exists."; \
	fi

.PHONY: all clean fclean re release .gitignore compile_ssl create_cert

create_cert:
	@if [ ! -f certs/cert.pem ]; then \
		mkdir -p certs; \
		openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout certs/key.pem -out certs/cert.pem -subj "/C=CA/ST=Catalonia/L=Mikasa/O=42/OU=42/CN=localhost"; \
	else \
		echo "Certificate already exists."; \
	fi

compile_ssl:
	cd third_party
	wget https://www.openssl.org/source/openssl-3.2.1.tar.gz
	tar -xzf openssl-3.2.1.tar.gz
	mv openssl-3.2.1 openssl
	rm openssl-3.2.1.tar.gz
	cd $(OPENSSL_SRC_DIR) && ./config no-shared --prefix=$(abspath $(OPENSSL_INSTALL_DIR)) && make && make install_sw
	@mkdir -p $(OPENSSL_BUILD_DIR)/include
	@mkdir -p $(OPENSSL_BUILD_DIR)/lib
	cp $(OPENSSL_INSTALL_DIR)/libssl.a $(OPENSSL_BUILD_DIR)/lib
	cp $(OPENSSL_INSTALL_DIR)/libcrypto.a $(OPENSSL_BUILD_DIR)/lib
	cp -r $(OPENSSL_INSTALL_DIR)/include $(OPENSSL_BUILD_DIR)/include
	@echo "OPENSSL DONE  "


-include $(DEP)
