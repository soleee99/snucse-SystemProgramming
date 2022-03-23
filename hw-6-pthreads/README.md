# Homework 6 - Parallel programming

In this homework, you are going to
*  make a simple data-parallel program multi-threaded
*  draw a performance graph in dependence of the number of threads

If you have not done so yet, fork this project into your own private namespace. Check that the project visibility is set to **private** (Settings -> General -> Visibility).

[[_TOC_]]


## Your Task

The handout includes a `Makefile` and a skeletton of a C file, `q3.c`. A binary of the solution is provided in `q3.reference` so that you can compare your code to ours.


### Make q3.c multi-threaded

Modify the function `compute()` in q3.c to perform the operation on the vectors with the desired number of `nthread` threads. To do so, you have to define a struct to hold the input/output parameters to the thread as well as a thread start routine that calls `kernel()` with the desired `low` and `high` limits.

Note that the return value of `compute()` must not change, no matter with how many threads the operation is performed.

### Determine the performance graph for your machine

Now use your program to determine the performance graph on your machine (or the virtual machine). Run the program with 1, 2, 4, ..., 8192, and 16384 threads and record the printed average performance.

The number of threads is provided as a command-line argument to q3.

```bash 
$ ./q3.reference 4
Initializing vectors (vector length: 4194304)...done.
Running experiments with 4 threads:
  Run 1...done.
  Run 2...done.
  Run 3...done.
  Run 4...done.
  Run 5...done.
  Run 6...done.
  Run 7...done.
  Run 8...done.

Results for 4 threads:
  Run   1:   0.352444 sec,   sum: 9147441278815155575
  Run   2:   0.365517 sec,   sum: 9147441278815155575
  Run   3:   0.372343 sec,   sum: 9147441278815155575
  Run   4:   0.336197 sec,   sum: 9147441278815155575
  Run   5:   0.335150 sec,   sum: 9147441278815155575
  Run   6:   0.336430 sec,   sum: 9147441278815155575
  Run   7:   0.334213 sec,   sum: 9147441278815155575
  Run   8:   0.336132 sec,   sum: 9147441278815155575

  Average:   0.346054 sec
  Stddev:    0.014397 sec
```

A sample graph is shown in the handout for homework #6 (available on eTL).

