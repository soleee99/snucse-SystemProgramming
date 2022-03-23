//--------------------------------------------------------------------------------------------------
// Homework #6                             Fall 2020                             System Programming
//
/// @file
/// @brief settings and function declarations
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
#ifndef __Q3_H__
#define __Q3_H__


#define NRUN                   8   ///< number of runs (incl. warm-up run)
#define MAX_THREAD         16384   ///< upper limit of #threads
#define NELEM        4*1024*1024L  ///< vector length
#define INTENSITY            512   ///< computational intensity of kernel (the larger the higher)


/// @brief get number of threads from command line. Filters out invalid/extreme values.
/// @parm argc number of command line arguments
/// @parm argv command line arguments
/// @retval long number of threads
long get_nthread(int argc, char *argv[]);


/// @brief return an allocated and optionally initialized vector. Aborts on errors.
/// @param initialize if not 0, initialize vector with random numbers
/// @retval int* vector
int* get_vector(int initialize);


/// @brief computation kernel. Performs an operation on vector @a A and @a B and stores the
///        result in vector @a C from indices @a low (included) to @a high (not included).
/// @parm C result vector
/// @parm A first input vector
/// @parm B second input vector
/// @parm low lower bound (included)
/// @param high higher bound (not included)
/// @retval long sum of processed elements of result vector C
long kernel(int *C, const int *A, const int *B, long low, long high);


#endif // __Q3_H__
