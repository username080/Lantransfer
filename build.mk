CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -Isocket -g
LDFLAGS = 

SRC_DIR = src
INC_DIR = include
SOCKET_DIR = socket
OBJ_DIR = obj

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/server.c \
       $(SRC_DIR)/client.c \
       $(SRC_DIR)/config.c \
       $(SRC_DIR)/protocol.c \
       $(SRC_DIR)/utils.c \
       $(SRC_DIR)/cJSON.c \
       $(SOCKET_DIR)/socket.c

OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))

TARGET = lantransfer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SOCKET_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
