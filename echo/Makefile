#---------------------------------------------------------------------------------------------------
# System Programming                    Echo Server/Client                               Fall 2020
#
# Makefile
#
# GNU make documentation: https://www.gnu.org/software/make/manual/make.html
#

#--- variable declarations

# C compiler and compilation flags
CC=gcc
CFLAGS=-Wall -O2 -g
DEPFLAGS=-MMD -MP

# make sure SOURCES includes ALL source files required to compile the project
SOURCES=$(wildcard *.c)
COMMON=common.o net.o

# derived variables
DEPS=$(SOURCES:.c=.d)



#--- rules
.PHONY: all doc clean

all: echoclient echoserveri echoserverp echoservert

echoserveri: echoserveri.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^

echoserverp: echoserverp.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^

echoservert: echoservert.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

echoclient: echoclient.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ -c $<

-include $(DEPS)

doc:
	doxygen doc/Doxyfile

clean:
	rm -rf echoclient echoserver{i,p,t} *.o *.d doc/html
