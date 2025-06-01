CC = clang
# Flags:
# -Wall              (Enable all routinely used warnings)
# -Wextra            (Enable additional warnings not covered by -Wall)
# -Wpedantic         (Warn on uses of non-standard C language extensions)
# -Werror            (Treat all warnings as errors, forcing fixes)
# -g                 (Include debugging information in the executable)
# -std=c99           (Compile according to the C99 language standard)
# -fsanitize=address (Enable AddressSanitizer for runtime memory error detection)
CFLAGS = -Wall -Wextra -Wpedantic -Werror -g -std=c99 -fsanitize=address
LDFLAGS = -fsanitize=address

# Source files that are part of the unity build (these are #included in UNITY_FILE)
# This list is for dependency tracking.
COMPONENT_SOURCES = main.c process.c arena.c
# Project-specific header files that, if changed, should trigger a rebuild.
PROJECT_HEADERS = process.h arena.h

# The main C file that #includes the others for the unity build.
UNITY_FILE = unity_build.c

# Name of the final executable
TARGET = cheat

# Makefile pls do not look for files with these names :)
.PHONY: all clean

# Default rule
all: $(TARGET)

# Rule to build the target executable from the unity file.
# The target depends on the unity file itself, the C files it includes,
# and any project headers to ensure changes in them trigger a rebuild.
$(TARGET): $(UNITY_FILE) $(COMPONENT_SOURCES) $(PROJECT_HEADERS)
	$(CC) $(CFLAGS) -o $@ $(UNITY_FILE) $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -f $(TARGET) # Only need to remove the final target
