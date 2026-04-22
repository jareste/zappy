# -=-=-=-=-    COLOURS -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- #

DEF_COLOR       = \033[0;39m
YELLOW          = \033[0;93m
CYAN            = \033[0;96m
GREEN           = \033[0;92m
BLUE            = \033[0;94m
RED             = \033[0;91m

# -=-=-=-=-    PROJECT -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- #

NAME            := client
CXX             := c++
PROFILE         ?= debug

# -=-=-=-=-    DIRECTORIES -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- #

SRCDIR          := srcs
OBJDIR          := .obj/$(PROFILE)
DEPDIR          := .dep/$(PROFILE)
INCDIR          := incs

OPENSSL_DIR     := ../server/third_party/openssl_build

TEST_DIR        := tests
TEST_OBJDIR     := .test_obj/$(PROFILE)
TEST_DEPDIR     := .test_dep/$(PROFILE)
TEST_BINARY     := run_tests_$(PROFILE)

LIB_DIR         := libs
GTEST_DIR       := $(LIB_DIR)/googletest
GTEST_REPO      := https://github.com/google/googletest.git
GTEST_LIB       := $(GTEST_DIR)/build/lib/libgtest.a
GTEST_MAIN_LIB  := $(GTEST_DIR)/build/lib/libgtest_main.a
GMOCK_LIB       := $(GTEST_DIR)/build/lib/libgmock.a

# -=-=-=-=-    SOURCES -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- #

MAIN_SRC        := main.cpp
PROT_SRC		:= protocol/MessageParser.cpp protocol/Sender.cpp
AGENT_SRC		:= agent/Behavior.cpp agent/Agent.cpp agent/Navigator.cpp
HELPER_SRC		:= helpers/Parser.cpp helpers/Logger.cpp
NET_SRC			:= net/FrameCodec.cpp net/SecureSocket.cpp net/TcpSocket.cpp net/TlsContext.cpp net/WebsocketClient.cpp
ALL_SRC         := $(MAIN_SRC) $(PROT_SRC) $(AGENT_SRC) $(HELPER_SRC) $(NET_SRC)

SRCS            := $(addprefix $(SRCDIR)/, $(ALL_SRC))
OBJS            := $(addprefix $(OBJDIR)/, $(ALL_SRC:.cpp=.o))
DEPS            := $(addprefix $(DEPDIR)/, $(ALL_SRC:.cpp=.d))

TEST_SRCS       := $(wildcard $(TEST_DIR)/unit/*.cpp) $(wildcard $(TEST_DIR)/integration/*.cpp)
TEST_OBJS       := $(patsubst $(TEST_DIR)/%.cpp,$(TEST_OBJDIR)/%.o,$(TEST_SRCS))
TEST_DEPS       := $(patsubst $(TEST_DIR)/%.cpp,$(TEST_DEPDIR)/%.d,$(TEST_SRCS))

TESTABLE_SRCS   := $(filter-out $(SRCDIR)/main.cpp,$(SRCS))
TESTABLE_OBJS   := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(TESTABLE_SRCS))

# -=-=-=-=-    FLAGS -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- #

INCLUDES        := -I$(INCDIR) -I$(SRCDIR) -I$(OPENSSL_DIR)/include
DEPFLAGS        := -MMD -MP
GTEST_INCLUDES  := -I$(GTEST_DIR)/googletest/include
GMOCK_INCLUDES  := -I$(GTEST_DIR)/googlemock/include
TEST_INCLUDES   := -I$(TEST_DIR)

COMMON_CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -pedantic $(INCLUDES) -DOPENSSL_API_COMPAT=30000
DEBUG_CXXFLAGS  := -g3 -O0
RELEASE_CXXFLAGS:= -g -O2 -DNDEBUG

ifeq ($(PROFILE),debug)
    CXXFLAGS := $(COMMON_CXXFLAGS) $(DEBUG_CXXFLAGS)
else ifeq ($(PROFILE),release)
    CXXFLAGS := $(COMMON_CXXFLAGS) $(RELEASE_CXXFLAGS)
else
    $(error Unknown PROFILE='$(PROFILE)'. Use PROFILE=debug or PROFILE=release)
endif

LDFLAGS         := -L$(OPENSSL_DIR)/lib -lssl -lcrypto -ldl -lm -lpthread
ALL_LIBS        :=
TEST_CXXFLAGS   := $(CXXFLAGS) $(GTEST_INCLUDES) $(GMOCK_INCLUDES) $(TEST_INCLUDES)
TEST_LDFLAGS    := -L$(OPENSSL_DIR)/lib -lpthread -lssl -lcrypto -ldl -lm

# -=-=-=-=-    RULES -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- #

all: $(NAME)

$(NAME): $(OBJS) Makefile
	@echo "$(CYAN)Linking $(NAME) [$(PROFILE)]...$(DEF_COLOR)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME) $(LDFLAGS) $(ALL_LIBS)
	@echo "$(GREEN)Build complete: $(NAME)$(DEF_COLOR)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$*.d)
	@echo "$(YELLOW)Compiling $<...$(DEF_COLOR)"
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@ -MF $(DEPDIR)/$*.d

run: all
	./$(NAME)

run-client: all
	./$(NAME) -n team1 -p 8674 -h 127.0.0.1

check: re
	valgrind --leak-check=full --show-leak-kinds=definite,indirect --track-origins=yes --num-callers=40 ./$(NAME)

# -=-=-=-=-    TEST TARGETS -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- #

check_gtest:
	@if [ ! -f "$(GTEST_LIB)" ] || [ ! -f "$(GTEST_MAIN_LIB)" ] || [ ! -f "$(GMOCK_LIB)" ]; then \
		echo "$(YELLOW)Google Test not ready. Bootstrapping local dependency...$(DEF_COLOR)"; \
		mkdir -p $(LIB_DIR); \
		if [ -d "$(GTEST_DIR)" ] && [ ! -d "$(GTEST_DIR)/googletest" ]; then \
			echo "$(YELLOW)Existing gtest directory looks incomplete. Recreating...$(DEF_COLOR)"; \
			rm -rf "$(GTEST_DIR)"; \
		fi; \
		if [ ! -d "$(GTEST_DIR)" ]; then \
			git clone --depth 1 --branch v1.14.0 $(GTEST_REPO) $(GTEST_DIR); \
		fi; \
		cd $(GTEST_DIR) && mkdir -p build && cd build && \
		cmake -DCMAKE_CXX_STANDARD=20 .. && \
		make -j4; \
		echo "$(GREEN)Google Test/Mock built successfully$(DEF_COLOR)"; \
	else \
		echo "$(GREEN)Google Test/Mock already built$(DEF_COLOR)"; \
	fi

test: check_gtest $(TEST_BINARY)
	@echo "$(CYAN)Running tests...$(DEF_COLOR)"
	./$(TEST_BINARY)

test-valgrind: check_gtest $(TEST_BINARY)
	@echo "$(CYAN)Running tests under valgrind...$(DEF_COLOR)"
	valgrind --track-origins=yes --leak-check=full ./$(TEST_BINARY)

ci-test:
	@echo "$(CYAN)Running CI test pipeline [$(PROFILE)]...$(DEF_COLOR)"
	@$(MAKE) PROFILE=$(PROFILE) fclean
	@$(MAKE) PROFILE=$(PROFILE) all
	@$(MAKE) PROFILE=$(PROFILE) test

retest: fclean test

$(TEST_BINARY): $(TESTABLE_OBJS) $(TEST_OBJS) $(CJSON_LIB) $(GTEST_LIB) $(GTEST_MAIN_LIB) $(GMOCK_LIB)
	@echo "$(YELLOW)Linking test binary...$(DEF_COLOR)"
	@$(CXX) -o $@ $(TESTABLE_OBJS) $(TEST_OBJS) $(CJSON_LIB) $(GTEST_MAIN_LIB) $(GTEST_LIB) $(GMOCK_LIB) $(TEST_LDFLAGS)
	@chmod +x $@
	@echo "$(GREEN)Test binary created: $(TEST_BINARY)$(DEF_COLOR)"

$(TEST_OBJDIR)/%.o: $(TEST_DIR)/%.cpp | $(TEST_OBJDIR) $(TEST_DEPDIR)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(TEST_DEPDIR)/$*.d)
	@$(CXX) $(TEST_CXXFLAGS) $(DEPFLAGS) -MF $(TEST_DEPDIR)/$*.d -c $< -o $@
	@echo "$(BLUE)Compiled test: $<$(DEF_COLOR)"

$(TEST_OBJDIR):
	@mkdir -p $(TEST_OBJDIR)/unit $(TEST_OBJDIR)/integration

$(TEST_DEPDIR):
	@mkdir -p $(TEST_DEPDIR)/unit $(TEST_DEPDIR)/integration

# -=-=-=-=-    CLEANING -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- #

clean:
	@echo "$(RED)Cleaning object files...$(DEF_COLOR)"
	@rm -rf .obj .dep .test_obj .test_dep

fclean: clean
	@echo "$(RED)Cleaning binaries...$(DEF_COLOR)"
	@rm -rf $(LIB_DIR)
	@rm -f $(NAME) $(TEST_BINARY)
	@rm -rf $(TEST_OBJDIR)
	@rm -f $(NAME) run_tests_debug run_tests_release

cleanlibs:
	@echo "$(RED)Cleaning libraries...$(DEF_COLOR)"
	@rm -rf $(LIB_DIR)

re: fclean all

-include $(DEPS)
-include $(TEST_DEPS)

.PHONY: all run run-client check clean fclean cleanlibs re test test-valgrind ci-test retest check_gtest
