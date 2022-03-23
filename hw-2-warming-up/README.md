# Homework 2 - Warming Up

In this homework, you are going to
*  learn the basic code editing cycle with Git
*  learn the basics of GNU make and
*  write a simple C program

If you have not done so yet, fork this project into your own private namespace. Check that the project visibility is set to **private** (Settings -> General -> Visibility).

[[_TOC_]]


## Introduction to Git

If you are unfamiliar with Git, have a look at the [Introduction to Git](INTRO_GIT.md).

## Introduction to GNU Make

If you are unfamiliar with GNU Make, have a look at the [Introduction to GNU Make](INTRO_MAKE.md).


## Your Task

The handout includes a `Makefile` and a skeletton of a C file, `make.c`. A binary of the solution is provided in `hw2.reference` so that you can compare your code to ours.

### Task 1

Modify `main.c` such that it prints all command line arguments as follows:
```bash 
$ ./hw2.reference hello there
Command line arguments (3 total):
   0: './hw2.reference'
   1: 'hello'
   2: 'there'
```

The number of arguments is provided in `argc`, the arguments themselves in `argv[]` of main:
```C
...
int main(int argc, char *argv[])
...
```

Add a loop after the first `TODO` in the code that first prints the string `Command line arguments (# total):` where # is replaced with the actual number of arguments. Then, add a loop (for or while) that runs through all arguments from `i=0...argc-1` and prints each argument `argv[i]`.

In the shell, compile your program using `make`. Stage the modified file and commit the solution to task 1 to your local Git repository.

### Task 2

In this second task, we want to add up all the numbers provided on the command line and print their sum. For example
```bash
$ ./hw2.reference 2020 9 10
Command line arguments (4 total):
   0: './hw2.reference'
   1: '2020'
   2: '9'
   3: '10'

Sum of arguments: 2030.
```

You can assume that all arguments (except argument 0 which always contains the filename of the program) are integer numbers. No error checking is needed.

To convert a string to an integer, use the `atoi()` function. You can learn how to use `atoi()` from the man pages: `man atoi`.

Add a loop that goes from `i=1..argc-1` and adds up the integer value of each argument. At the end, print the sum.

In the shell, compile your program using `make`. Stage the modified file and commit the solution to task 2 to your local Git repository.

### Task 3

We want to implement the addition in our own custom math library. For that purpose, we define a header file and implement the corresponding C file.

The header file is given here, you can simply copy-paste it into `mathlib.h`:
```C
//--------------------------------------------------------------------------------------------------
// System Programming                     Homework #2                                    Fall 2020
//
/// @file
/// @brief math library header file
/// @author <yourname>

/// @brief returns the sum of two integer values
/// @param a term a
/// @param b term b
/// @retval sum of a + b
int add(int a, int b);
```
In case you are wondering what the funny symbols in the comments are: these are Doxygen-style comments. We will use Doxygen to comment our lab code.

Implement the function `add()` in `mathlib.c`, then replace the addition (`+`) in `main.c` with a call to `add()`. Do not forget to include the header file of our math library at the beginning of `main.c`:
```C
#include "mathlib.h"
```

Now compile your project first using `gcc` directly
```bash 
$ gcc -o hw2 main.c mathlib.c
```
If all is good you will see no compilation errors. If there are errors, fix them.

Next try to compile the project using make. First run `make mrproper` to clean all files, then compile:
```bash
$ make mrproper
rm -f main.o main.d
rm -f hw2
$ make
set -e; rm -f main.d; gcc -std=c99 -Wall -O2 -MM main.c | sed 's/\(main\)\.o[ :]*/\1.o main.d: /' > main.d
gcc -std=c99 -Wall -O2   -c -o main.o main.c
gcc -std=c99 -Wall -O2 -o hw2 main.o
main.o: In function `main':
main.c:(.text.startup+0x7): undefined reference to `add'
collect2: error: ld returned 1 exit status
Makefile:70: recipe for target 'hw2' failed
make: *** [hw2] Error 1
```

The problem is that the Makefile does not contain a rule to compile mathlib.c. Open the Makefile and inspect it. You need to add `mathlib.c` in one location only to make the compilation process work:
```bash
$ make 
set -e; rm -f mathlib.d; gcc -std=c99 -Wall -O2 -MM mathlib.c | sed 's/\(mathlib\)\.o[ :]*/\1.o mathlib.d: /' > mathlib.d
set -e; rm -f main.d; gcc -std=c99 -Wall -O2 -MM main.c | sed 's/\(main\)\.o[ :]*/\1.o main.d: /' > main.d
gcc -std=c99 -Wall -O2   -c -o main.o main.c
gcc -std=c99 -Wall -O2   -c -o mathlib.o mathlib.c
gcc -std=c99 -Wall -O2 -o hw2 main.o mathlib.o
```

Test your work, then stage the modified and new files and commit the solution to task 3 to your local Git repository.


## Submission

If you have committed your work as instructed at the end of tasks 1-3, you can submit your solution by pushing your commits to the Gitlab server:
```bash
$ git push
Counting objects: 4, done.
Delta compression using up to 4 threads.
Compressing objects: 100% (3/3), done.
Writing objects: 100% (4/4), 379 bytes | 0 bytes/s, done.
Total 4 (delta 2), reused 0 (delta 0)
To https://teaching.csap.snu.ac.kr/yourname/hw-2-warming-up.git
   ec4fa12..ce69fec  master -> master
```


