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

# top-level rule to create the program
all: main

# compiling the source file
main: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS)

prompt: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS)

# cleaning everything that can be automatically recreated with "make"
clean:
	rm -f $(NAME) $(IGNORE)

# tar all files together
tar:
	tar cfv $(TAR) $(SRCS) $(MAKEFILE) $(NAME)
