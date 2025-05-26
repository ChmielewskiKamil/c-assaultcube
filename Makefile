CC = clang
CFLAGS = -Wall -Wextra -g
LDFLAGS =

# List all object files
OBJS = main.o process.o

# Name of the final executable
TARGET = cheat

# Default rule
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile each .c file into an object file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

