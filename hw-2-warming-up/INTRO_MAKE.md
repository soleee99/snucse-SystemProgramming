[[_TOC_]]

# Introduction to GNU Make

GNU Make is a compiler driver. A compiler driver checks which source code files have been modified and executes all necessary commmands to build a project. Many different compiler drivers exist (cmake, scons, ...), but we use GNU Make as it is (relatively) easy to use and available on (almost) all Linux distributions.

GNU Make is executed by typing the command `make`. The command expects a file called `Makefile` to be present in the current directory, then reads and follows the instructions in this Makefile. If no Makefile exists, make will complain.
```bash
$ make
make: *** No targets specified and no makefile found.  Stop.
```

Change into the directory containing your cloned homework and execute make. Make executes a series of commands to build the executable `hw2`
```bash
$ make
set -e; rm -f main.d; gcc -std=c99 -Wall -O2 -MM main.c | sed 's/\(main\)\.o[ :]*/\1.o main.d: /' > main.d
gcc -std=c99 -Wall -O2   -c -o main.o main.c
gcc -std=c99 -Wall -O2 -o hw2 main.o
```

If you execute make a second time, make knows that it is not necessary to rebuild `hw2` and prints
```bash
$ make
make: Nothing to be done for 'all'.
```

However, if a file was modified since `hw2` was last built, make will recompile the project
```bash
$ vi main.c
$ make
set -e; rm -f main.d; gcc -std=c99 -Wall -O2 -MM main.c | sed 's/\(main\)\.o[ :]*/\1.o main.d: /' > main.d
gcc -std=c99 -Wall -O2   -c -o main.o main.c
gcc -std=c99 -Wall -O2 -o hw2 main.o
```

## Makefile

The Makefile describes the rules with which a project is to be built. You can specify several so-called *targets* such as *compile* to compile the project or *clean* to remove all temporary files. Typical targets are *all* to build the entire project, *clean* to remove unneeded temporary files, and *mrproper* to clean everything, including generated binaries.

The Makefile syntax is very powerful and complex relationships can be expressed. We only discuss the basics here, for a complete reference refer to the [GNU Make documentation](https://www.gnu.org/software/make/manual/). 

For our purposes, we use only variable definitions and rules. A variable definition has the form
```make
VARIABLE=value
```
This simply assigns `value` to `VARIABLE`. Variables can be referenced by using the syntax `$(VARIABLE)`.

A rule has the form
```make
target: depenencies
    command
    ...
```

To define a rule called *clean* that deletes all object files (`*.o`), we can write
```make
clean:
    rm -f *.o
```

The following is a rule that generates the executable file `hw2` from the source code files `main.c` and `mathlib.c`
```make
hw2: main.c mathlib.c
    gcc -Wall -O2 -o hw2 main.c mathlib.c
```

The dependencies (`main.c mathlib`) after the colon inform make when it has to rebuild the target. In this concrete example, target `hw2` needs to be rebuilt whenever one of the dependent files, `main.c` or `mathlib.c` have been modified. 


Variables can be used in rules using the `$(VARIABLE)` syntax. Also, it would be nice if we didn't have to specify the target and dependencies again in the command (`gcc -o hw2 main.c mathlib.c`) but could instead refer to them. Indeed, GNU make provides a number of so-called *automatic variables* that are very convenient when writing rules.

The rule above can be rewritten as follows
```make
CC=gcc
CFLAGS=-Wall -O2

hw2: main.c mathlib.c
    $(CC) $(CFLAGS) -o $@ $^
```
`$@` and `$^` are automatic variables. `$@` refers to the target (`hw2`) and `$^` to all dependencies (`main.c mathlib.c`).

The handout includes a Makefile that contains several variable definitions and rules. You will extend the Makefile slightly.

