#---------------------------------------------------------------------------------------------------
# Lab 2: I/O Lab                          Fall 2020                               System Programming
#
# Makefile
#
# GNU make documentation: https://www.gnu.org/software/make/manual/make.html
#
# Typically, the only thing you need to modify in this Makefile is the list of source files.
#

#--- variable declarations

# C compiler and compilation flags
CC=gcc
CFLAGS=-std=c99 -Wall -Wno-stringop-truncation -O2 -g
DEPFLAGS=-MMD -MP

# make sure SOURCES includes ALL source files required to compile the project
SOURCES=dirtree.c
TARGET=dirtree

# derived variables
OBJECTS=$(SOURCES:.c=.o)
DEPS=$(SOURCES:.c=.d)


#--- rules
.PHONY: doc

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ -c $<

-include $(DEPS)

doc: $(SOURES) $(wildcard $(SOURCES:.c=.h))
	doxygen doc/Doxyfile

clean:
	rm -f $(OBJECTS) $(DEPS)

mrproper: clean
	rm -rf $(TARGET) doc/html
