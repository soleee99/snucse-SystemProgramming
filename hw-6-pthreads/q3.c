//--------------------------------------------------------------------------------------------------
// Homework #6                             Fall 2020                             System Programming
//
/// @file
/// @brief pthread data-parallel demo
/// @author <your name> (<your email>)
/// @section changelog Change Log
/// 2020/11/01 Bernhard Egger created
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
//--------------------------------------------------------------------------------------------------
#define _XOPEN_SOURCE 600   // required for clock_gettime()

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "q3.h"

#define sqr(x)  ((x)*(x))


/// @brief compute res = op(v1, v2) for all elements of the vectors with @a nthread threads
/// @param res result vector
/// @param v1 input vector 1
/// @param v2 input vector 2
/// @param nthread number of threads
/// @retval sum of result vector elements

void* thread_routine(void *vargp){
  
}

long compute(int *res, const int *v1, const int *v2, int nthread)
{
  //
  // TODO
  //
  // modify the code here to distribute the entire workload 0...NELEM-1 to 'nthread' threads.
  //
  // You will need to define a data structure to hold the thread arguments and a new
  // function that acts as the thread's start_routine.
  for(int i = 0 ; i < nthread ; i++){
    pthread_t tid;

  }
  struct{
    long low;
    long high;
  }
  return kernel(res, v1, v2, 0, NELEM);
}


/// @brief program entry point. Accepts an optional command line parameter
///        indicating the number of threads to perform the computation with.
int main(int argc, char *argv[])
{
  // determine number of threads
  long nthread = get_nthread(argc, argv);


  // get & initialize vectors
  printf("Initializing vectors (vector length: %ld)...", NELEM); fflush(stdout);
  srand(1522800);
  int *A = get_vector(1);
  int *B = get_vector(1);
  int *C = get_vector(0);
  printf("done.\n");


  // run experiments
  struct {
    long result;
    struct timespec time;
  } run[NRUN];

  printf("Running experiments with %ld threads:\n", nthread);
  for (long r=0; r<NRUN; r++) {
    printf("  Run %ld...", r+1); fflush(stdout);

    // start time
    struct timespec ts, te;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // compute
    run[r].result = compute(C, A, B, nthread);

    // end time
    clock_gettime(CLOCK_MONOTONIC, &te);

    printf("done.\n");

    // compute & record time difference
    te.tv_nsec -= ts.tv_nsec;
    if (te.tv_nsec < 0) { te.tv_nsec += 1000000000; te.tv_sec--; }
    te.tv_sec  -= ts.tv_sec;

    run[r].time = te;
  }


  // print results of single runs, followed by the average and the standard deviation
  double time[NRUN];
  double total = 0.0;

  printf("\nResults for %ld threads:\n", nthread);
  for (long r=0; r<NRUN; r++) {
    printf("  Run %3ld: %3ld.%06ld sec,   sum: %ld\n",
        r+1, 
        run[r].time.tv_sec, run[r].time.tv_nsec / 1000,
        run[r].result);

    time[r] = (double)run[r].time.tv_sec + (double)run[r].time.tv_nsec / 1000000000;
    total += time[r];
  }

  double average = total / NRUN;
  double sesq = 0.0;
  for (long r=0; r<NRUN; r++) sesq += sqr(time[r] - average);
  double stdev = sqrt(sesq / NRUN);

  printf("\n"
         "  Average: %10.6f sec\n"
         "  Stddev:  %10.6f sec\n",
         average, stdev);


  // that's all, folks!
  return EXIT_SUCCESS;
}


