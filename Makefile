CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
SRC_DIR = src
BUILD_DIR = build
TARGET = ratio

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/lexer.c

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Run with example
run: $(TARGET)
	./$(TARGET) examples/hello.ratio

# Phony targets
.PHONY: all clean run
