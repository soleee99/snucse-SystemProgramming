#---------------------------------------------------------------------------------------------------
# Lab 6: Network Lab                        Fall 2020                           System Programming
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
CFLAGS=-Wall -Wno-stringop-truncation -O2 -g -lpthread
DEPFLAGS=-MMD -MP

# make sure SOURCES includes ALL source files required to compile the project
SOURCES=$(wildcard *.c)
TARGET=mcdonalds client
COMMON=net.o

# derived variables
OBJECTS=$(SOURCES:.c=.o)
DEPS=$(SOURCES:.c=.d)


#--- rules
.PHONY: doc

all: mcdonalds client

mcdonalds: mcdonalds.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^

client: client.o $(COMMON)
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
