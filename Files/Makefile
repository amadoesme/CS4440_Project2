# Students: Esmeralda Amado, Cristian Hernandez Juan, Gustavo Trejo
# Class: CS 4440-01 (Operating Systems)
# A single makefile that compiles all C files in the current directory and creates
# an executable for each C file. 


# C compiler and flags for warnings and debugging  
CC = gcc
CFLAGS  = -Wall -Wextra -g -std=gnu11


LDFLAGS = -pthread -lrt

# Finds all c files in the current directory. 
SOURCES = $(wildcard *.c)

# executables have the same name as the .c file but without the extension
PROGRAMS = $(SOURCES:.c=)

# Builds every program that was found
all: $(PROGRAMS)

# Rule for converting .c into exe of the same name 
%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)


# Prints the sources and executables that were discovered 
list:
	@echo "Sources found:  $(SOURCES)"
	@echo "Programs built: $(PROGRAMS)"


#Removes the built executables so the next build starts fresh, rm ignores the error when nothing has been built yet 
clean:
	-rm -f $(PROGRAMS)


.PHONY: all clean list