//--------------------------------------------------------------------------------------------------
// System Programming                      Introspection Lab                             Fall 2020
//
/// @file
/// @brief introspection of pthread mutexes to detect deadlocks through library interpositioning
/// @author Junsung Yook <junsung@csap.snu.ac.kr>
/// @section changelog Change Log
/// 2020/11/09 Junsung Yook created
/// 2020/11/16 Bernhard Egger cleanup
///
/// @section license_section License
/// Copyright (c) 2020, Computer Systems and Platforms Laboratory, SNU
/// All rights reserved.
///
/// Redistribution and use in source and binary forms, with or without modification, are permitted
/// provided that the following conditions are met:
///
/// - Redistributions of source code must retain the above copyright notice, this list of condi-
///   tions and the following disclaimer.
/// - Redistributions in binary form must reproduce the above copyright notice, this list of condi-
///   tions and the following disclaimer in the documentation and/or other materials provided with
///   the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
/// IMPLIED WARRANTIES,  INCLUDING, BUT NOT LIMITED TO,  THE IMPLIED WARRANTIES OF MERCHANTABILITY
/// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
/// CONTRIBUTORS BE LIABLE FOR ANY DIRECT,  INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSE-
/// QUENTIAL DAMAGES (INCLUDING,  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
/// LOSS OF USE, DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED AND ON ANY THEORY OF
/// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
/// DAMAGE.
/// TODO: code modified by Jinsol Park (2018-14000)
//--------------------------------------------------------------------------------------------------


// Deadlock Detector
// =================
//
// This module implements a Deadlock Detector. Deadlocks are detected when a cycle forms (would
// form) among the threads holding a lock and requesting one. In the simplest case, two threads
// each hold a lock an request the one the other threads holds as follows:
//
//  Time      Thread 1             Thread 2
//    1       lock mutex 1
//    2                            lock mutex 2
//    3                            try to lock mutex 1
//    4       try to lock mutex 2
//                ---------  deadlock ---------
//
// At this point, thread 1 and 2 are deadlocked.
//
// A deadlock can occur between an arbitrary number of threads >= 2. An example involving four
// threads where the mutex allocation forms a circle is given below:
//
//  Time      Thread 1             Thread 2             Thread 3             Thread 4
//    1       lock mutex 1         lock mutex 2         lock mutex 3         lock mutex 4
//    2       try to lock mutex 2  try to lock mutex 3  try to lock mutex 4  try to lock mutex 1
//                                  ---------  deadlock ---------
//
// To detect deadlocks, we need to track the resource allocations (mutexes) of each thread. In
// particular, we need to maintain a list of mutexes currently held by a thread and what mutex
// it is trying to acquire.
// Checking for a deadlock requires following the chain of threads that block on a mutex. If 
// that sequence leads to a cycle, we have a deadlock.
//
//
// Nested doubly linked list organization
// --------------------------------------
//
// We provide the necessary data structures to track resource usage (mutex locks) of the
// individual threads. It consists of two doubly-linked lists. The first list links all threads.
// Each thread contains a second list that links all resources held by that thread:
//
//  Thread head
//    ^
//    |
//    v
//    Thread 1 -- resources: r1 <-> r2 <-> r3
//    ^
//    |
//    v
//    Thread 2 -- resources: r4 <-> r5
//    ^
//    |
//    v
//  Thread tail
//
//
// Visualization
// -------------
//
//  Thread head
//    ^
//    |
//    V
//    Thread 1 {
//      Thread Id:       1001
//      Mutexes held:      m2 <-> m3 <-> m1 <----,
//      Requested mutex:   m4                    |
//    }                     ^                    |
//    ^                     |                    |
//    |                     `------,             |
//    V                            |             |
//    Thread 2 {                   |             |
//      Thread Id:       1002      v             |
//      Mutexes held:      m5 <-> m4 <-> m6      |
//      Requested mutex:   m9                    |
//    }                     ^                    |
//    ^                     |                    |
//    |                     `-------------,      |
//    V                                   |      |
//    Thread 3 {                          |      |
//      Thread Id:       1003             v      |
//      Mutexes held:      m7 <-> m8 <-> m9      |
//      Requested mutex:   m1 <------------------'
//    }
//    ^
//    |
//    V
//  Thread tail
//

#define _GNU_SOURCE
#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

/// @name structures
/// @{

/// @brief general node element to implement a doubly-linked list
typedef struct __node {
  struct __node *prev;                                        ///< pointer to previous node
  struct __node *next;                                        ///< pointer to next node
  void *data;                                                 ///< pointer to node data
} Node;

/// @brief resource data
typedef struct __resource_data {
  pthread_mutex_t *mutex;                                     ///< mutex
} ResourceData;

typedef pid_t tid_t;                                          ///< thread ID returned by gettid()

/// @brief thread data
typedef struct __thread_data {
  tid_t tid;                                                  ///< thread ID
  pthread_mutex_t *req_mutex;                                 ///< requested mutex
  Node resource_list_head;                                    ///< head of resource list
  Node resource_list_tail;                                    ///< tail of resource list
} ThreadData;

/// @brief Used to pass information to the intercepted thread's start routine (routine_wrapper)
typedef struct {
  void* (*start_routine)(void*);                              ///< thread start routine
  void *arg;                                                  ///< start routine arguments
} PthreadStart;
/// @}


/// @name global variables
/// @{

struct timespec start_point;                                   ///< process start time
struct timespec end_point;                                     ///< process end time (deadlock time)

static pthread_mutex_t t_list_mtx = PTHREAD_MUTEX_INITIALIZER; ///< mutex to protect thread list
static pthread_mutex_t ref_mtx = PTHREAD_MUTEX_INITIALIZER;    ///< mutex to protect resource list

static Node thread_list_head;                                  ///< head of thread list
static ThreadData tail_td = {.tid = INT_MAX };                 ///< thread list tail marker
static Node thread_list_tail = { .data = &tail_td };           ///< tail for thread list

static ResourceData tail_rd = { .mutex = NULL };               ///< resource list tail marker

static int (*pthread_mutex_lock_orig)(pthread_mutex_t *) = NULL;
static int (*pthread_mutex_unlock_orig)(pthread_mutex_t *) = NULL;
static int (*pthread_create_orig)(pthread_t*,
            __const pthread_attr_t*,
            void* (*start_routine)(void*), void*) = NULL;
/// @}


/// @name Macro definitions
/// @{

#define LOCK(x)   if (pthread_mutex_lock_orig(x) != 0) PANIC("LOCK: %s", strerror(errno))
#define UNLOCK(x) if (pthread_mutex_unlock_orig(x) != 0) PANIC("UNLOCK: %s", strerror(errno))
/// @}


/// @brief Prints an error message and terminates the process. Does not return.
/// @param fmt printf format string
/// @param ... variadic parameters for @a fmt
__attribute__((noreturn))
static void PANIC(const char *fmt, ...)
{
  fprintf(stderr, "[PANIC] ");

  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);

  fprintf(stderr, "\n");

  exit(EXIT_FAILURE);
}

/// @}


//--------------------------------------------------------------------------------------------------
/// @name common list operations
/// @{

/// @brief Insert a new node with payload @a data after node @a n
/// @param n node to insert new node after
/// @param data payload
void insert_node_after(Node *n, void *data)
{
  Node *nn = malloc(sizeof(Node));
  nn->data = data;
  nn->prev = n;
  nn->next = n->next;
  n->next->prev = nn;
  n->next = nn;
}

/// @brief Remove node @a n
/// @param n node to remove
void remove_node(Node *n)
{
  n->prev->next = n->next;
  n->next->prev = n->prev;
  free(n);
}

/// @brief Find node that matches comparator @a cmp
/// @param head head of list
/// @param cmd comparator function
/// @param arg argument passed to comparator's @arg argument
/// @retval Node* matching node or tail of list
Node* find_node(Node *head, int (*cmp)(Node *n, void *arg), void *arg)
{
  Node *this = head;

  while ((this->next != NULL) && !cmp(this->next, arg)) this = this->next;

  return this->next;
}

/// @}


//--------------------------------------------------------------------------------------------------
/// @name thread list primitive operations
/// @{

/// @brief Comparator for @a find_node(). Returns true if the tid matches
/// @param n node to compare
/// @param arg pointer to tid to match
/// @retval 1 node's tid matches tid provided in arg
/// @retval 0 otherwise
int t_list_cmp (Node *n, void *arg )
{
  return ((ThreadData*)n->data)->tid == *(tid_t *)arg;
}

/// @brief Comparator for @a find_node(). Returns true if the tid of the node is > the tid in arg
/// @param n node to compare
/// @param arg pointer to tid to compare to
/// @retval 1 node's tid is bigger than tid provided in arg
/// @retval 0 otherwise
int t_list_next_cmp (Node *n, void *arg )
{
    return ((ThreadData*)n->data)->tid > *(tid_t *)arg;
}

/// @brief Initialize the thread list
void init_list_thread(void)
{
  thread_list_head.prev = NULL;
  thread_list_head.next = &thread_list_tail;
  thread_list_tail.prev = &thread_list_head;
  thread_list_tail.next = NULL;
}

/// @brief Find a thread by its @a tid
/// @param tid thread ID
/// @retval Node* containing ThreadData of @a tid
/// @retval NULL thread not found
Node* find_thread(tid_t tid)
{
  LOCK(&t_list_mtx);
  Node *n = find_node(&thread_list_head, &t_list_cmp, &tid);
  UNLOCK(&t_list_mtx);

  return n;
}

/// @brief Find a thread by its @ tid and return the data node
/// @param tid thread ID
/// @retval ThreadData* ThreadData for @a tid
/// @retval NULL thread not found
ThreadData* find_thread_data(tid_t tid)
{
  Node *n = find_thread(tid);

  return n != NULL ? (ThreadData*)n->data : NULL;
}

/// @brief Insert a thread into the ordered thread list
/// @param tid thread ID
/// @retval ThreadData* ThreadData for @a tid
/// @retval NULL thread nod found
ThreadData* insert_thread_orderly(tid_t tid)
{
  ThreadData *td = malloc(sizeof(ThreadData));
  td->tid = tid;

  LOCK(&t_list_mtx);
  Node *n = find_node(&thread_list_head, &t_list_next_cmp, &tid);
  insert_node_after(n->prev, td);
  UNLOCK(&t_list_mtx);

  return td;
}

/// @brief Remove thread @a tid from the thread list
/// @param tid thread ID
void remove_thread(tid_t tid)
{
  LOCK(&t_list_mtx);
  Node *n = find_node(&thread_list_head, &t_list_cmp, &tid);
  if (n != NULL) {
    free(n->data);
    remove_node(n);
  }
  UNLOCK(&t_list_mtx);
}

/// @}


//--------------------------------------------------------------------------------------------------
/// @name resource list primitive operations
/// @{

/// @brief Comparator for @a find_node(). Returns true if the mutex matches
/// @param n node to compare
/// @param arg pointer to mutex to match
/// @retval 1 node's mutex matches mutex provided in arg
/// @retval 0 otherwise
int r_list_cmp(Node *n, void *arg)
{
  return ((ResourceData*)n->data)->mutex == *(pthread_mutex_t **)arg;
}

/// @brief Initialize a resource list list
/// @param head head node
/// @param tail tail node
void init_list_resrc(Node *head, Node *tail)
{
    head->prev = NULL;
    head->next = tail;
    tail->prev = head;
    tail->next = NULL;
    tail->data = &tail_rd;
}

/// @brief Find a resource by its @a mutex
/// @param head head of list
/// @param mutex mutex to search for
/// @retval Node* containing ResourceData of @a mutex
/// @retval NULL resource not found
Node* find_resrc(Node *head, pthread_mutex_t *mutex)
{
  Node *n = find_node(head, &r_list_cmp, &mutex);
  return n;
}

/// @brief Insert an uninitialized resource node just before the end of a resource list
/// @param tail tail of list
/// @retval ResourceData* inserted ResourceData (uninitialized)
ResourceData* insert_resrc_last(Node *tail)
{
  ResourceData *rd = malloc(sizeof(ResourceData));
  insert_node_after(tail->prev, rd);
  return rd;
}

/// @brief Remove the resource node matching @a mutex
/// @param head head of list
/// @param mutex mutex to search & remove
void remove_resrc(Node *head, pthread_mutex_t *mutex)
{
  Node *n = find_node(head, &r_list_cmp, &mutex);
  if (n != NULL) {
    remove_node(n);
    free(n->data);
  }
}

/// @}


//--------------------------------------------------------------------------------------------------
/// @name prints information about deadlocks and where it has occurred
/// @{

/// @brief Print out information about a detected deadlockA
/// @param tid thread ID of thread trying to lock @a mutex
/// @param mutex mutex causing the deadlock
void print_deadlock_info(tid_t tid, pthread_mutex_t *mutex)
{
  printf("\n--Deadlock Detection--\n");

  clock_gettime(CLOCK_MONOTONIC, &end_point);

  struct timespec duration;
  duration.tv_sec = end_point.tv_sec - start_point.tv_sec;
  duration.tv_nsec = end_point.tv_nsec - start_point.tv_nsec;
  if (duration.tv_nsec < 0) {
    duration.tv_nsec += 1000000000;
    duration.tv_sec--;
  }

  printf("(Deadlock) Elapsed Time: %3ld.%ld sec\n", duration.tv_sec, duration.tv_nsec / 1000000);

  printf("\nResource Allocation: ");

  Node *tn = thread_list_head.next;
  while (tn->next) {
    ThreadData *td = tn->data;
    printf("\nThread ID %d: requested: %p; owns:",
           td->tid, td->tid==tid?mutex:td->req_mutex);

    Node *rn = td->resource_list_head.next;
    while (rn->next) {
      printf(" -> %p", ((ResourceData*)rn->data)->mutex);
      rn = rn->next;
    }

    tn = tn->next;
  }

  printf("\n\nCircular wait:\n");
  printf("*%d -> ", tid);

  do {
    printf("(%p) -> %d -> ", mutex, mutex->__data.__owner);

    ThreadData *td = find_thread_data(mutex->__data.__owner);
    if (td != NULL) {
      mutex = td->req_mutex;
      if (tid == mutex->__data.__owner) printf("(%p) -> *%d ", mutex, tid);
    } else {
      printf("*** invalid thread: %d ***\n", mutex->__data.__owner);
      break;
    }
  } while (tid != mutex->__data.__owner);
}

/// @brief Finds the physical offset into the program's executable object file for a given 
///        virtual address
/// @param va virtual address
//

void print_line_info(void *va)
{
  char program[PATH_MAX+1], elf[PATH_MAX+1];
  ssize_t len;
  unsigned long long base = 0;

  // determine own program name
  if ((len = readlink("/proc/self/exe", program, sizeof(program)-1)) < 0) {
    PANIC("%s", strerror(errno));
  }

  // iterate through list of mapped files to find location of program code in memory
  DIR *dp = NULL;
  struct dirent *entry = NULL;

  if ((dp = opendir("/proc/self/map_files")) == NULL) PANIC("%s", strerror(errno));

  while ((entry = readdir(dp))) {
    // ignore "." and ".." entries
    if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) continue;

    // follow link in entry->d_name and store it in elf
    char *link = NULL;
    if (asprintf(&link, "/proc/self/map_files/%s", entry->d_name) < 0) PANIC("Cannot asprintf");
    len = readlink(link, elf, sizeof(elf)-1);
    elf[len] = '\0';
    free(link);

    // did we find the program?
    if (strcmp(program, elf) == 0) {
      // the link name has the format <VA start>-<VA end>. We are interested in <VA start>
      base = strtoull(entry->d_name, NULL, 16);
      break;
    }
  }

  closedir(dp);

  // print information
  if (base != 0) {
    printf("\n\nDeadlock in '%s' at address %p.\n\n", elf, va - base);
  } else {
    printf("\n\nELF file for deadlock address %p not found.\n\n", va);
  }
}


//--------------------------------------------------------------------------------------------------
void* routine_wrapper(void* arg){                                     // run in newly created thread
  tid_t tid = gettid();                                               // get current 
  PthreadStart* pst = (PthreadStart*) arg;                            // cast void* type to (PthreadStart*) type

  ThreadData *td = insert_thread_orderly(tid);                        // returns the new thread node
  init_list_resrc(&td->resource_list_head, &td->resource_list_tail);  // initialize the resource list of that thread

  void* rtn = pst->start_routine(pst->arg);                           // call original thread start_routine
  remove_thread(tid);                                                 // remove thread from list

  return rtn;
}

/// @name pthread_create intercept
/// @{

/// @brief pthread_create intercept. See pthread_create(3) for arguments/return value
int pthread_create(pthread_t  *thread, __const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg)
{
  PthreadStart *pst = malloc(sizeof(PthreadStart));
  pst->arg = arg;                                     // assigning given parameter arg to pst->arg
  pst->start_routine = start_routine;                 // assigning given parameter start_routine to pst->start_routine
  
  // call the original pthread_create with another start routine function
  // call the original start_routine within the wrapper
  // pst will be passed on as an argument to routine_wrapper
  int rtn = pthread_create_orig(thread, attr, routine_wrapper, pst); 
  //free(pst);
  return rtn;

  //return EAGAIN;
}

/// @}


//--------------------------------------------------------------------------------------------------
/// @name pthread_mutex_lock/unlock intercept and deadlock detection
/// @{

/// @brief Check for cycles in the thread resource graph
/// @param tid thread ID
/// @param mutex mutex to check for
/// @retval owner of mutex or 0 if no thread currently holds the mutex
tid_t contain_cycle(tid_t tid, pthread_mutex_t *mutex)
{
  // Hint: mutex->__data.__owner contains the TID of the thread owning this mutex
  // find circular wait using resource allocation graph 
  // (already implemented using thread doubly linked list / nested...)

  if(mutex == NULL) return 0;                         // check if the given mutex is NULL

  ThreadData* td;
  while((mutex->__data.__owner) != 0){                // if the __data.__owner is 0, means there is no thread owning the mutex
    if (tid == mutex->__data.__owner){                // we arrived back at ourselves. stop & return tid
      return mutex->__data.__owner;
    }else{                                            // need to circle around more
      td = find_thread_data(mutex->__data.__owner);   // get the owner thread of the mutex
      mutex = td->req_mutex;                          // update 'mutex' to the required mutex of the thread
      if(mutex == NULL) break;                        // if the thread does not require any mutex
    }
  }
  //no cycle is detected
  //fprintf(stderr, "[CONTAIN_CYCLE]: no circular wait\n");
  return 0;
}

/// @brief pthread_mutex_lock intercept. See pthread_mutex_loc(3) for arguments/return value
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  // check whether current resource allocation graph contains a cycle or not
  // if it contains cycle, return EDEADLOCK error code
  tid_t tid = gettid();               // tid of current thread

  // calls the contain_cycle function and checks if circular wait cnd exists
  LOCK(&ref_mtx);                     // start of critical section
  if(contain_cycle(tid, mutex)){
    print_deadlock_info(tid, mutex);
    // __builtin_return_address(0) obtains the return address of the current frame
    // value 1 would mean the caller of the curr function
    print_line_info(__builtin_return_address(0));
    UNLOCK(&ref_mtx);                 // UNLOCK the mutex before returning.
    return EDEADLK;
  }
   
  ThreadData* curr_td = find_thread_data(tid); 
  curr_td->req_mutex = mutex;                   // update the current thread's required mutex to 'mutex'
  UNLOCK(&ref_mtx);                             // end of critical section
  
  int rtn = pthread_mutex_lock_orig(mutex);     // call the original pthread_mutex_lock function
  
  LOCK(&ref_mtx);                               // start of critical section
  curr_td->req_mutex = NULL;                    // now the thread is the owner of the mutex, so change required mutex to NULL
  ResourceData *rd = insert_resrc_last(&curr_td->resource_list_tail); // add a new ResourceData to resource list of thread
  rd->mutex = mutex;                            // the mutex of that resource is the currently locked mutex
  UNLOCK(&ref_mtx);                             // end of critical section
  
  return rtn;

}

/// @brief pthread_mutex_lock intercept. See pthread_mutex_loc(3) for arguments/return value
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  tid_t tid = gettid();                         // get tid of current thread

  int rtn = pthread_mutex_unlock_orig(mutex);   // call the original pthread_mutex_unlock function

  LOCK(&ref_mtx);                               // start of critical section
  remove_resrc(&(find_thread_data(tid)->resource_list_head), mutex);  // remoce the mutex from the resource list of thread
  UNLOCK(&ref_mtx);                             // end of critical section
  return rtn;
}

/// @}


//--------------------------------------------------------------------------------------------------
/// @name intercept of program beginning / end
/// @{

/// @brief Intercept of CRT's start routine
int __libc_start_main(int (*main)(int, char **, char **), int argc, char **argv,
        void (*init) (void), void (*fini) (void), void (*rtld_fini) (void), void (*stack_end))
{
  // find addresses of original (non-intercepted) functions
  typeof(&__libc_start_main) __libc_start_main_orig = dlsym(RTLD_NEXT, "__libc_start_main");
  if (__libc_start_main_orig == NULL) PANIC("Cannot find '__libc_start_main'");

  // initialize pthread_create_orig, pthread_mutex_lock, pthread_mutex_unlock
  // called by loader, and it calls user main function.
  // we can do something before calling the main, and do some other things about main
  // if we change main function's address to our function intercept,
  // in that function we can do something else and call the original function in the end.
  char *error;
  pthread_create_orig = dlsym(RTLD_NEXT, "pthread_create");               // getting the real pthread_create function
  if((error = dlerror()) != NULL) PANIC("%s", error);

  pthread_mutex_lock_orig = dlsym(RTLD_NEXT, "pthread_mutex_lock");       // getting the real pthread_mutex_lock function
  if((error = dlerror()) != NULL) PANIC("%s", error);

  pthread_mutex_unlock_orig = dlsym(RTLD_NEXT, "pthread_mutex_unlock");   // getting the real pthread_mutex_unlock function
  if((error = dlerror()) != NULL) PANIC("%s", error);

  // initialize thread list
  init_list_thread();

  // record process start time
  clock_gettime(CLOCK_MONOTONIC, &start_point);

  // call original __libc_start_main
  return __libc_start_main_orig(main, argc, argv, init, fini, rtld_fini, stack_end);
}

/// @}
