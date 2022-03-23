#---------------------------------------------------------------------------------------------------
# Lab 3: Memory Lab                       Fall 2020                               System Programming
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
CFLAGS=-Wall -Wno-stringop-truncation -O2 -g
DEPFLAGS=-MMD -MP

# make sure SOURCES includes ALL source files required to compile the project
SOURCES=mm_test.c memmgr.c dataseg.c
TARGET=mm_test

# derived variables
OBJECTS=$(SOURCES:.c=.o)
DEPS=$(SOURCES:.c=.d)


#--- rules
.PHONY: doc

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

mm_driver: memmgr.o dataseg.o
	$(CC) $(CFLAGS) -o $@ $^ obj/blocklist.o obj/mm_driver.o

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ -c $<

-include $(DEPS)

doc: $(SOURES) $(wildcard $(SOURCES:.c=.h))
	doxygen doc/Doxyfile

clean:
	rm -f $(OBJECTS) $(DEPS)

mrproper: clean
	rm -rf $(TARGET) mm_driver doc/html
