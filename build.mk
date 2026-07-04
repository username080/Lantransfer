CC = gcc
CFLAGS = -Wall -Wextra -g

# The new domain-driven directories
CORE_DIR = core
NET_DIR = network
SERVER_DIR = server
CLIENT_DIR = client
UTILS_DIR = utils
OBJ_DIR = obj

# Find all C files automatically
SRCS = $(wildcard $(CORE_DIR)/*.c) \
       $(wildcard $(NET_DIR)/*.c) \
       $(wildcard $(SERVER_DIR)/*.c) \
       $(wildcard $(CLIENT_DIR)/*.c) \
       $(wildcard $(UTILS_DIR)/*.c)

# Convert source file names into object file paths
OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))

TARGET = lantransfer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Dynamic compilation rules for each directory
$(OBJ_DIR)/%.o: $(CORE_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(NET_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SERVER_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(CLIENT_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(UTILS_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
