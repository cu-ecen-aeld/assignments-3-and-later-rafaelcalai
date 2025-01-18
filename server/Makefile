# Compiler and tools (defaults for standalone builds, overridden by Yocto)
CC ?= gcc
AR ?= ar
STRIP ?= strip

# Compiler flags
CFLAGS ?= -Wall -Werror -g
LDFLAGS ?= -pthread -lrt -lm

# Target binary name
TARGET = aesdsocket

# Source and object files
SRCS = aesdsocket.c
OBJS = $(SRCS:.c=.o)

# Default target
.PHONY: all
all: $(TARGET)

# Build the target binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)

# Install target (for use with Yocto's do_install)
.PHONY: install
install:
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/
