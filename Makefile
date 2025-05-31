CC = clang
# Flags:
# -Wall              (Enable all routinely used warnings)
# -Wextra            (Enable additional warnings not covered by -Wall)
# -Wpedantic         (Warn on uses of non-standard C language extensions)
# -Werror            (Treat all warnings as errors, forcing fixes)
# -g                 (Include debugging information in the executable)
# -std=c99           (Compile according to the C99 language standard)
# -fsanitize=address (Enable AddressSanitizer for runtime memory error detection)CFLAGS = -Wall -Wextra -Wpedantic -Werror -g -std=c99 -fsanitize=address
LDFLAGS = -fsanitize=address

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

