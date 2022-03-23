#---------------------------------------------------------------------------------------------------
# Homework #2                             Fall 2020                               System Programming
#
# Makefile
#
# GNU make documentation: https://www.gnu.org/software/make/manual/make.html
# A very brief introduction is provided below.

#--- variable declarations

# C compiler and compilation flags
CC=gcc
CFLAGS=-std=c99 -Wall -O2

# project related information: SOURCES lists all source files of the project, TARGET the desired
# name of the generated binary. OBJECTS and DEPS are generated automatically by pattern substitution
SOURCES=main.c mathlib.c
TARGET=hw2
OBJECTS=$(SOURCES:.c=.o)
DEPS=$(SOURCES:.c=.d)



#--- rules

# rules tell GNU Make how to build something and have the form
#   LHS: RHS
#     CMD
#     [CMD]
# where LHS is the target to be generated, RHS the files the target depends on. CMD lists the
# command(s) that is/are to be executed when LHS needs to be rebuilt.
#
# Example: After variable substitition in the rule below
#
# 	$(TARGET): $(OBJECTS)
# 		$(CC) $(CFLAGS) -o $@ $^
#
# we get
#
# 	hw2: main.o
# 		gcc -Wall -O2 -o hw2 main.o
#
# $@ and $^ are so-called automatic variables; more information can be found in the GNU make
# documentation at https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html
#
# This rule thus states that the target 'hw2' depends on main.o, i.e., whenever main.o changes
# the target has to be rebuilt. This is then done by executing the gcc command. 



# The first rule is the default rule, i.e., when typing 'make' without any further arguments, so
# we need to make sure it points to something useful. SOP is to use 'all:' as the first rule so
# that 'make all' and 'make' have the same effect.

all: $(TARGET)

# --- do not modify ----
# automatic dependency generation
# the following include and rule automatically generate a list of dependencies for a C file.
# If, for example, the file 'main.c' includes a header file 'compute.h', then main needs to
# be recompiled every time main.c or compute.h change.

-include $(DEPS)

%.d: %.c
	set -e; rm -f $@; $(CC) $(CFLAGS) -MM $< | sed 's/\($*\)\.o[ :]*/\1.o $@: /' > $@
# --- end of do not modify ----

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJECTS) $(DEPS)

mrproper: clean
	rm -f $(TARGET)
