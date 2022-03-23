# Lab 5 - Introspection Lab

In this lab, we implement a tool that detect a deadlock in a multi-thread program.

You will learn
   * how to intercept function calls that already exist in the target program during the execution time by dynamic linking for preloaded library.
   * how to find circular waits for detecting deadlock.
   * how to organize a nested doubly linked list.
   * how to get a virtual address at calling function and convert it to offset of the executable.

[[_TOC_]]


## Important Dates

| Date | Description |
|:---  |:--- |
| Monday, November 16, 18:30 | Introspection Lab hand-out |
| Monday, November 16, 18:30 | Introspection Lab session 1 |
| Monday, November 23, 18:30 | Introspection Lab session 2 |
| Sunday, November 29, 12:00 | Submission deadline |


## Logistics

### Hand-out
The lab is handed out via https://teaching.csap.snu.ac.kr/sysprog/2020-fall/lab-5-introspection-lab.

Start by forking the lab into your namespace and making sure the lab visibility is set to private. Read the instructions here carefully. Then clone the lab to your local computer and get to work. 

### Submission

Commit and push your work frequently to avoid data loss. Once you are happy with your solution and want to submit it for grading, create a tag called "Submission". The timestamp of the "Submission" tag is considered your submission time.

To create a tag, go to your Introspection lab repository on https://teaching.csap.snu.ac.kr/ and navigate to Repository -> Tags. Enter "Submission" as the Tag name then hit "Create tag". You can leave the other fields empty.  

If you later discover an error and want to update your submission, you can delete the "Submission" tag, fix your solution, and create a new "Submission" tag. If that happens _after_ the submission deadline, you also need to email the TAs so that they are aware of the update to your submission.


## Deadlock Detector Overview

###  LD_PRELOAD 

LD_PRELOAD is an optional environment variable that contains one or more paths to shared libraries to be loaded by the loader before other shared libraries, including the C runtime library (libc.so that call __libc_start_main). This is called preloading a library. Preloading a library means that its functions will be used before others of the same name in later libraries. This enables library functions to be intercepted and replaced. 

In the following example, the dynamic executable target_prog is loaded, followed by the shared object libpreload.so. 

#### Set an environment variable
```
$LD_PRELOAD=./libpreload.so target_prog
```
#### Intercept a function call
```
ssize_t read(int fd, void *buf, size_t count)
{
    ssize_t (*lread)(int fd, void *buf, size_t count) = dlsym(RTLD_NEXT, "read");
    
    /****
    do somthing
    ***/
    return lread(fd, buf, count);
}
```




###  Resource-Allocation Graph 

We can find a deadlock by traversing a Resource-Allocation Graph

#### Deadlock Detection with Resource-Allocation graph that contains a cycle
```
  Thread head
    ^
    |
    V
    Thread 1 {
      Thread Id:       1001
      Mutexes held:      m2 <-> m3 <-> m1 <----,
      Requested mutex:   m4                    |
    }                     ^                    |
    ^                     |                    |
    |                     `------,             |
    V                            |             |
    Thread 2 {                   |             |
      Thread Id:       1002      v             |
      Mutexes held:      m5 <-> m4 <-> m6      |
      Requested mutex:   m9                    |
    }                     ^                    |
    ^                     |                    |
    |                     `-------------,      |
    V                                   |      |
    Thread 3 {                          |      |
      Thread Id:       1003             v      |
      Mutexes held:      m7 <-> m8 <-> m9      |
      Requested mutex:   m1 <------------------'
    }
    ^
    |
    V
  Thread tail
```

To do this, You should manage a Nested doubly linked list, a list of threads and a nested sublist pointing to resources owned by that thread.



## Deadlock Detector Specification 

### Operation

1. Show the Elapsed Time from the start when deadlock is found.
2. Manage the list of existing threads that would be modified according to thread creation and termination and sorted by thread ID. Also, as a nested sublist of nodes in the list above, mutex resources owned by the thread must be managed in a list in the order they were allocated.
3. EDEADLK is returned when deadlock caused by circular waiting is detected.
4. The virtual address of the return address of 'pthread_mutex_lock' causing the deadlock is obtained and translated into address within the executable.

### Scope

The target program only uses pthread_mutex_lock to acquire mutex. Also, this tool aims at concurrency control between multiple threads created by a process with pthread_create(pthread library).

### Output

#### Elapsed Time

This prints out elapsed time on Deadlock.
```
(Deadlock) Elapsed Time:   0.609 sec
```

#### Resource Allocation
It shows the sorted list of thread IDs, the currently requested and owned resources address.
```
Resource Allocation: 
Thread ID 10204; requested: 0x55db73bbd150, owns: -> 0x55db73bbd470 -> 0x55db73bbd128
Thread ID 10205; requested: 0x55db73bbd178, owns: -> 0x55db73bbd2e0 -> 0x55db73bbd150
Thread ID 10206; requested: 0x55db73bbd1a0, owns: -> 0x55db73bbd600 -> 0x55db73bbd178
Thread ID 10207; requested: 0x55db73bbd1c8, owns: -> 0x55db73bbd308 -> 0x55db73bbd1a0
Thread ID 10208; requested: 0x55db73bbd1f0, owns: -> 0x55db73bbd380 -> 0x55db73bbd1c8
Thread ID 10209; requested: 0x55db73bbd128, owns: -> 0x55db73bbd6a0 -> 0x55db73bbd1f0
```

#### Circular Wait
When a deadlock occurs, it shows the circular wait through the thread ID and the requested resource address.
```
Circular Wait: 
*10208 -> (0x55db73bbd1f0) -> 10204 -> (0x55db73bbd150) -> 10205 -> (0x55db73bbd178) -> 10206 -> (0x55db73bbd1a0) -> 10207 -> (0x55db73bbd1c8) -> *10208 
```

#### Return address in the executable

The output of the address in the executable is for the return address from calling 'pthread_mutex_lock'.
To translates virtual address into the address of the executable, the '/proc/(<pid> or self)/map_files/' directory or the '/proc/(<pid> or self)/maps' system file can be refered to.
```
Deadlock in '2020-2/labs/05.Introspection/tools/mutex_cc' at address 0x1ab8.

```

You can use addr2line tool to translates offset into file names and line numbers.
```
$ addr2line -f -e ./mutex_cc 0x2727
_Z9do_stuff6Pv
/home/csap/TA/2020_2_SP/mutex_cc.c:553
```

#### Multiple Deadlock Detection

If deadlocks are detected sequentially, all of them are displayed. Note that in the second Deadlock Detection, thread 10209 is terminated due to first deadlock 

```
--Deadlock Detection--
(Deadlock) Elapsed Time:   0.301 sec

Resource Allocation: 
Thread ID 10204; requested: 0x55db73bbd150; owns: -> 0x55db73bbd470 -> 0x55db73bbd128
Thread ID 10205; requested: 0x55db73bbd178; owns: -> 0x55db73bbd2e0 -> 0x55db73bbd150
Thread ID 10206; requested: 0x55db73bbd1a0; owns: -> 0x55db73bbd600 -> 0x55db73bbd178
Thread ID 10207; requested: 0x55db73bbd1c8; owns: -> 0x55db73bbd308 -> 0x55db73bbd1a0
Thread ID 10208; requested: 0x55db73bbd1f0; owns: -> 0x55db73bbd380 -> 0x55db73bbd1c8
Thread ID 10209; requested: 0x55db73bbd128; owns: -> 0x55db73bbd6a0 -> 0x55db73bbd1f0

Circular Wait: 
*10209 -> (0x55db73bbd128) -> 10204 -> (0x55db73bbd150) -> 10205 -> (0x55db73bbd178) -> 10206 -> (0x55db73bbd1a0) -> 10207 -> (0x55db73bbd1c8) -> 10208 -> (0x55db73bbd1f0) -> *10209 

Deadlock in '2020-2/labs/05.Introspection/tools/mutex_cc' at address 0x2b40.


--Deadlock Detection--
(Deadlock) Elapsed Time:   1.2 sec

Resource Allocation: 
Thread ID 10204; requested: 0x55db73bbd150; owns: -> 0x55db73bbd470 -> 0x55db73bbd128
Thread ID 10205; requested: 0x55db73bbd178; owns: -> 0x55db73bbd2e0 -> 0x55db73bbd150
Thread ID 10206; requested: 0x55db73bbd1a0; owns: -> 0x55db73bbd600 -> 0x55db73bbd178
Thread ID 10207; requested: 0x55db73bbd1c8; owns: -> 0x55db73bbd308 -> 0x55db73bbd1a0
Thread ID 10208; requested: 0x55db73bbd470; owns: -> 0x55db73bbd380 -> 0x55db73bbd1c8 -> 0x55db73bbd1f0

Circular wait: 
*10208 -> (0x55db73bbd470) -> 10204 -> (0x55db73bbd150) -> 10205 -> (0x55db73bbd178) -> 10206 -> (0x55db73bbd1a0) -> 10207 -> (0x55db73bbd1c8) -> *10208 

Deadlock in '2020-2/labs/05.Introspection/tools/mutex_cc' at address 0x2727.

```

### Memory Leak

A memory leak occurs When the allocated memory to create a node in the linked list is not released. Note that when removing a node from the linked list, the allocated memory should be released.


## Handout Overview

The handout contains the following files and directories

| File/Directory | Description |
|:---  |:--- |
| README.md | this file | 
| Makefile | Makefile driver program |
| libintrospect.c | Skeleton for libintrospect.c. Implement your solution by editing this file. |
| .gitignore | Tells git which files to ignore |
| doc/ | Doxygen instructions, configuration file, and auto-generated documentation |
| tools/ | Tools to make a various concurrency control situations for testing |



### Tools

The `tools` directory contains tools to make a various concurrency control situations to test your solution.

| File/Directory | Description |
|:---  |:--- |
| mutex_cc | Test Target Program to be able to make a various concurrency control situations.

Invoke `mutex_cc` with a script file to run test program that uses mutex for multithreading concurrency control. 

```
TAG
l (mutex index): lock operation
u (mutex index): unlock operation
t (microseconds): sleep operation
```
Between each operation, the default delay is 100 milliseconds.
Each line represents a thread.

You can write the contents of the script. The script below represents a situation where there is a circular wait between 6 threads.
```
l 1 l 2 u 2 u 1
l 2 l 3 u 3 u 2
l 3 l 4 u 4 u 3
l 4 l 5 u 5 u 4
l 5 l 6 u 6 u 5
l 6 l 1 u 1 u 6
```

'cc_1.dat' is the script passed as an argument to mutex_cc..
```
LD_PRELOAD=./libintrospect.so ./mutex_cc ./cc_1.dat 
```

## Your Task

Your task is to implement Deadlock Detecor according to the specification above.

### Design

In a first step, write down the logical steps of your program on a sheet of paper. We will look into design together during the first lab session.

**Recommendation**: Think about the logical steps yourself. 
The design is the most difficult and important phase in any project - and also the phase that requires the most practice and sets apart hobby programmers from experts.

### Implementation

Once you have designed the outline of your implementation, you can start implementing it. We provide a skeleton file to help you get started.

The skeleton provides printing and listes related functions.

You have to implement the following two parts:
1. Implement the operations to find the circular waits contained in it.
2. Implement the functions of the same prototype as one of the function intercepted in the target program. This functions are implemented for deadlock detection and printing out result by intercepting some function calls related to thread creation and mutex allocation and release.


#### C header files  
The skeleton code alread includes all the necessary C header files.

#### C library calls

Knowing which library function to call is difficult at the beginning in every programming language. To help you get started, we provide a list of C library calls / system calls grouped by topic that you may find helpful to solve this lab.

<table>
  <thead>
    <tr>
      <th align="left">Topic</th>
      <th align="left">C library call</th>
      <th align="left">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="2">
        String operations
      </td>
      <td>
        <code>strcmp()</code>
      </td>
      <td>
        compare two strings
      </td>
    </tr>
    <tr>
      <td>
        <code>strtoull()</code>
      </td>
      <td>
        convert a string to an unsigned long integer
      </td>
    </tr>
    <tr>
      <td rowspan="4">
        Directory management
      </td>
      <td>
        <code>opendir()</code>
      </td>
      <td>
        open a directory to enumerate its entries
      </td>
    </tr>
    <tr>
      <td>
        <code>closedir()</code>
      </td>
      <td>
        close an open directory
      </td>
    </tr>
    <tr>
      <td>
        <code>readdir()</code>
      </td>
      <td>
        read next entry from directory
      </td>
    </tr>
    <tr>
      <td>
        <code>readlink()</code>
      </td>
      <td>
        read value of a symbolic link
      </td>
    </tr>    
  </tbody>
</table>

#### Final words

It is too late to start next week. TA will deal with the enquiry mail received this week in the next lab session. 

Do not despair - we will give detailed instructions during the lab sessions and provide individual help so that each of you can finish this lab. After completing this lab, you can call yourself a system programmer. Inexperienced still, but anyway a system programmer.

<div align="center" style="font-size: 1.75em;">

**Happy coding!**
</div>
