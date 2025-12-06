.DEFAULT_GOAL := all
CC := cc
CFLAGS := -Wall -W -O2 -Iinclude
LDFLAGS :=

SRC_DIR := src
OBJ_DIR := obj
INCLUDE_DIR := include
BUILD_DIR := build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXEC = $(BUILD_DIR)/cq

all: $(EXEC)

$(EXEC): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(EXEC)
	
test:
	$(EXEC) ./test_data.csv

.PHONY: all clean up env down build logs test repl