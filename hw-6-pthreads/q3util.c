//--------------------------------------------------------------------------------------------------
// Homework #6                             Fall 2020                             System Programming
//
/// @file
/// @brief utility functions
/// @author Bernhard Egger <bernhard@csap.snu.ac.kr>
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
#define _XOPEN_SOURCE 500   // required for clock_gettime()
#include <stdio.h>
#include <stdlib.h>

#include "q3.h"


long get_nthread(int argc, char *argv[])
{
  long res = 1;

  // get nthread from command line if provided
  if (argc > 1) res = atoi(argv[1]);

  // filter out invalid & extreme values
  if (res < 1) res = 1;
  if (res > MAX_THREAD) res = MAX_THREAD;

  return res;
}


int* get_vector(int initialize)
{
  int *res = malloc(NELEM * sizeof(int));
  if (res == NULL) {
    perror("Cannot allocate vector"); exit(EXIT_FAILURE);
  }

  if (initialize) {
    for (long i=0; i<NELEM; i++) {
      res[i] = (int)random() % INTENSITY;
    }
  }

  return res;
}


/// @brief iterative fibonacci algorithm (runtime: O(n))
/// @parm  n fibonacci number to compute
/// @retval long n-th fibonacci number
long fib(long n)
{
  long res=0, p=1, pp=0;

  while (n > 0) {
    res = p + pp;
    pp = p;
    p = res;
    n--;
  }

  return res;
}


long kernel(int *C, const int *A, const int *B, long low, long high)
{
  //
  // DO NOT MODIFY THIS FUNCTION.
  // Your modifications should go into the function 'compute' below.
  //
  long sum = 0;

  for (long i=low; i<high; i++) {
    long l = fib(A[i]) + fib(B[i]);
    C[i] = l;
    sum += l;
  }

  return sum;
}


