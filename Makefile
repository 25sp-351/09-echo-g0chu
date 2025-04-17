# Executable name
TARGET = echo_server

# Source files
SRCS = main.c threads.c

# Object files (derived from SRCS)
OBJS = $(SRCS:.c=.o)

# Compiler and flags
CC = gcc
# -Wall: Enable warnings
# -g: Add debug symbols
# -pthread: Needed for pthreads (compile and link)
CFLAGS = -Wall -g -pthread

# Default target
all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(CFLAGS) $(OBJS) # -pthread needed for linking

# Dependencies (object files depend on source and headers)
main.o: main.c threads.h
threads.o: threads.c threads.h

# Implicit rule for .c -> .o compilation is usually sufficient
# It uses $(CC) and $(CFLAGS) by default. If defined explicitly:
# %.o: %.c
#	$(CC) -c $(CFLAGS) $< -o $@

# Clean rule (remove generated files)
clean:
	rm -f $(TARGET) $(OBJS)

# Phony targets (not files)
.PHONY: all clean