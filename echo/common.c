//--------------------------------------------------------------------------------------------------
// System Programming                   Echo Server/Client                               Fall 2020
//
/// @file  common.h
/// @brief helper functions for various purposes
///
/// @author Bernhard Egger <bernhard@csap.snu.ac.kr>
/// @section changelog Change Log
/// 2016/10/14 Bernhard Egger created
/// 2017/11/24 Bernhard Egger added put/get_line functions
/// 2017/12/06 Bernhard Egger moved networking code to net.c/h & cleanup
/// 2020/11/25 Bernhard Egger cleanup & minor bugfixes
///
/// @section license_section License
/// Copyright (c) 2016-2020, Computer Systems and Platforms Laboratory, SNU
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

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

void syntax(const char *msg, const char *syntax)
{
  if (msg) fprintf(stderr, "%s\n\n", msg);
  if (syntax) fprintf(stderr, "Syntax: %s\n", syntax);
  else fprintf(stderr, "Syntax error.\n");

  exit(EXIT_FAILURE);
}

void error(const char *msg)
{
  if (msg) fprintf(stderr, "Error: %s\n", msg);
  else fprintf(stderr, "Error.\n");

  exit(EXIT_FAILURE);
}
