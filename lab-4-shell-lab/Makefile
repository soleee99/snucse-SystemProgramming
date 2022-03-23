#---------------------------------------------------------------------------------------------------
# Lab 4: Shell Lab                       Fall 2020                               System Programming
#
# Makefile
#


DRIVER = ./sdriver.pl
TSH = ./tsh
TSHREF = ./tshref
TSHARGS = "-p"
CC = gcc
CFLAGS = -Wall -O2 -m32
FILES = $(TSH) ./myspin ./mysplit ./mystop ./myint


all: $(FILES)




##################
# Regression tests
##################

# Run tests using the student's shell program
test01:
	$(DRIVER) -t trace/trace01.txt -s $(TSH) -a $(TSHARGS)
test02:
	$(DRIVER) -t trace/trace02.txt -s $(TSH) -a $(TSHARGS)
test03:
	$(DRIVER) -t trace/trace03.txt -s $(TSH) -a $(TSHARGS)
test04:
	$(DRIVER) -t trace/trace04.txt -s $(TSH) -a $(TSHARGS)
test05:
	$(DRIVER) -t trace/trace05.txt -s $(TSH) -a $(TSHARGS)
test06:
	$(DRIVER) -t trace/trace06.txt -s $(TSH) -a $(TSHARGS)
test07:
	$(DRIVER) -t trace/trace07.txt -s $(TSH) -a $(TSHARGS)
test08:
	$(DRIVER) -t trace/trace08.txt -s $(TSH) -a $(TSHARGS)
test09:
	$(DRIVER) -t trace/trace09.txt -s $(TSH) -a $(TSHARGS)
test10:
	$(DRIVER) -t trace/trace10.txt -s $(TSH) -a $(TSHARGS)
test11:
	$(DRIVER) -t trace/trace11.txt -s $(TSH) -a $(TSHARGS)
test12:
	$(DRIVER) -t trace/trace12.txt -s $(TSH) -a $(TSHARGS)
test13:
	$(DRIVER) -t trace/trace13.txt -s $(TSH) -a $(TSHARGS)
test14:
	$(DRIVER) -t trace/trace14.txt -s $(TSH) -a $(TSHARGS)
test15:
	$(DRIVER) -t trace/trace15.txt -s $(TSH) -a $(TSHARGS)
test16:
	$(DRIVER) -t trace/trace16.txt -s $(TSH) -a $(TSHARGS)
test17:
	$(DRIVER) -t trace/trace17.txt -s $(TSH) -a $(TSHARGS)
test18:
	$(DRIVER) -t trace/trace18.txt -s $(TSH) -a $(TSHARGS)
test19:
	$(DRIVER) -t trace/trace19.txt -s $(TSH) -a $(TSHARGS)
test20:
	$(DRIVER) -t trace/trace20.txt -s $(TSH) -a $(TSHARGS)
test21:
	$(DRIVER) -t trace/trace21.txt -s $(TSH) -a $(TSHARGS)


# Run the tests using the reference shell program
rtest01:
	$(DRIVER) -t trace/trace01.txt -s $(TSHREF) -a $(TSHARGS)
rtest02:
	$(DRIVER) -t trace/trace02.txt -s $(TSHREF) -a $(TSHARGS)
rtest03:
	$(DRIVER) -t trace/trace03.txt -s $(TSHREF) -a $(TSHARGS)
rtest04:
	$(DRIVER) -t trace/trace04.txt -s $(TSHREF) -a $(TSHARGS)
rtest05:
	$(DRIVER) -t trace/trace05.txt -s $(TSHREF) -a $(TSHARGS)
rtest06:
	$(DRIVER) -t trace/trace06.txt -s $(TSHREF) -a $(TSHARGS)
rtest07:
	$(DRIVER) -t trace/trace07.txt -s $(TSHREF) -a $(TSHARGS)
rtest08:
	$(DRIVER) -t trace/trace08.txt -s $(TSHREF) -a $(TSHARGS)
rtest09:
	$(DRIVER) -t trace/trace09.txt -s $(TSHREF) -a $(TSHARGS)
rtest10:
	$(DRIVER) -t trace/trace10.txt -s $(TSHREF) -a $(TSHARGS)
rtest11:
	$(DRIVER) -t trace/trace11.txt -s $(TSHREF) -a $(TSHARGS)
rtest12:
	$(DRIVER) -t trace/trace12.txt -s $(TSHREF) -a $(TSHARGS)
rtest13:
	$(DRIVER) -t trace/trace13.txt -s $(TSHREF) -a $(TSHARGS)
rtest14:
	$(DRIVER) -t trace/trace14.txt -s $(TSHREF) -a $(TSHARGS)
rtest15:
	$(DRIVER) -t trace/trace15.txt -s $(TSHREF) -a $(TSHARGS)
rtest16:
	$(DRIVER) -t trace/trace16.txt -s $(TSHREF) -a $(TSHARGS)
rtest17:
	$(DRIVER) -t trace/trace17.txt -s $(TSHREF) -a $(TSHARGS)
rtest18:
	$(DRIVER) -t trace/trace18.txt -s $(TSHREF) -a $(TSHARGS)
rtest19:
	$(DRIVER) -t trace/trace19.txt -s $(TSHREF) -a $(TSHARGS)
rtest20: 
	$(DRIVER) -t trace/trace20.txt -s $(TSHREF) -a $(TSHARGS)
rtest21: 
	$(DRIVER) -t trace/trace21.txt -s $(TSHREF) -a $(TSHARGS)


# clean up
clean:
	rm -f $(FILES) *.o *~


