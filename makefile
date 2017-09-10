NAME = fLisp
DEBUG = -g
CFLAGS = $(DEBUG) -Wall -std=c99 -c 
LFLAGS = $(DEBUG) -Wall -ledit -o $(NAME)
SRCS = prompt.c
OBJS = prompt.o
TAR = $(NAME).tar
MAKEFILE = makefile
CC = gcc
IGNORE = *~ *.o
DEF = -D _WIN32
# Environment on which the compilation is aimed to: 1) linux 2) windows
OS ?= LINUX

# top-level rule to create the program
all: main

# compiling the source file
main: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS)

prompt.o: $(SRCS)
ifeq ($(OS), LINUX)
	$(CC) $(CFLAGS) $(SRCS)
else ifeq ($(OS), WIN32)
	$(CC) $(DEF) $(CFLAGS) $(SRCS)
endif

# cleaning everything that can be automatically recreated with "make"
clean:
	rm -f $(NAME) $(IGNORE)

# tar all files together
tar:
	tar cfv $(TAR) $(SRCS) $(MAKEFILE) $(NAME)
