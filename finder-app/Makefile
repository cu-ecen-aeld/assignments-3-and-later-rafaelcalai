# the compiler: gcc for C program, define as g++ for C++
  CC = gcc

  CROSS_COMPILE ?= 
  CC := $(CROSS_COMPILE)gcc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
  CFLAGS  = -g -Wall

  # the build target executable:
  TARGET = writer

  # Default target
  .PHONY: all
  all: $(TARGET)

  $(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

  # Clean target
  .PHONY: clean
  clean:
	$(RM) $(TARGET)