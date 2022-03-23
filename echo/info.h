//--------------------------------------------------------------------------------------------------
// System Programming                   Echo Server/Client                               Fall 2020
//
/// @file  info.h
/// @brief Doxygen info
///
/// @author Bernhard Egger <bernhard@csap.snu.ac.kr>
/// @section changelog Change Log
/// 2017/12/06 Bernhard Egger created
/// 2020/11/25 Bernhard Egger updates to documentation
///
/// @section license_section License
/// Copyright (c) 2017-2020, Computer Systems and Platforms Laboratory, SNU
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


///
/// @mainpage System Programming - Echo Server
///
/// This example demonstrates how to program a simple web service that reads strings from a network
/// socket and sends the same string in UPPERCASE back to the client.
///
/// Three versions of servers are provided:
/// - echoserveri.c: an iterative, blocking version of the server
/// - echoserverp.c: a concurrent multi-process version of the server
/// - echoservert.c: a concurrent multi-threaded version of the server
///
/// A client is provided in echoclient.c.
///
/// The code makes use of the System Programming Network library to simplify sending/receiving of 
/// text lines.
///
