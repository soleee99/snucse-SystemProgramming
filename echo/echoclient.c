//--------------------------------------------------------------------------------------------------
// System Programming                   Echo Server/Client                               Fall 2020
//
/// @file  echoclient.c
/// @brief echo client
///
/// @author Bernhard Egger <bernhard@csap.snu.ac.kr>
/// @section changelog Change Log
/// 2016/10/14 Bernhard Egger created
/// 2017/11/24 Bernhard Egger added put/get_line functions
/// 2017/12/06 Bernhard Egger cleanup
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

// standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// for networking
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"
#include "net.h"
#include "echo.h"

static char stx_str[] = "client <host> [<port>]";

static int sock_fd = -1;              ///< listening socket, closed in atexit()

/// @brief connect to @a host at port @a port
///
/// Returns the file descriptor of the connected socket
/// or aborts (does not return) on failure.
///
/// @param host hostname of server
/// @param port port of server
/// @retval int file descriptor of connected socket
int connect_to(char *host, uint16_t port)
{
  struct addrinfo *ai, *ai_it;
  int fd = -1;

  printf("Connecting to %s:%d...\n", host, port);

  //
  // get list of potential sockets
  //
  ai = getsocklist(host, port, AF_UNSPEC, SOCK_STREAM, 0, NULL);

  // iterate through potential addressinfo structs and try to connect to server
  ai_it = ai;
  while (ai_it != NULL) {
    printf("  trying "); dump_sockaddr(ai_it->ai_addr); printf("..."); fflush(stdout);

    fd = socket(ai_it->ai_family, ai_it->ai_socktype, ai_it->ai_protocol);
    if (fd != -1) {
      if (connect(fd, ai_it->ai_addr, ai_it->ai_addrlen) != -1) break; // success, leave
      close(fd);
    }
    printf("failed.\n");
    ai_it = ai_it->ai_next;
  }

  // ai_it == NULL -> connection failed, abort
  if (ai_it == NULL) error("Cannot connect.");

  freeaddrinfo(ai);

  printf("success.\n");
  return fd;
}

/// @brief main client routine: read input line from terminal, send it to
///        server and print reply
/// @param connfd connected socket
void run_client(int connfd)
{
  char *msg = NULL;
  size_t msg_len = 0;

  if (isatty(STDIN_FILENO)) printf("Press Ctrl-C to exit.\n");

  while (1) {
    ssize_t res;
    // read a line of input
    printf("[EchoClient] "); fflush(stdout);
    if ((res = getline(&msg, &msg_len, stdin)) > 0) {
      int res;

      // send line to server
      res = put_line(connfd, msg, msg_len);
      if (res < 0) printf("Error: cannot send data to server (%d).\n", res);
      else {
        // read server reply
        printf("[EchoServer] "); fflush(stdout);
        res = get_line(connfd, &msg, &msg_len);
        if (res <= 0) printf("Error: cannot read reply from server (%d).\n", res);
        else printf("%s", msg);
      }
    } else break;
  }

  printf("End of input reached.\n");
  free(msg);
}


/// @brief close main connection (atexit() handler)
void close_connection(void)
{
  if (sock_fd != -1) {
    close(sock_fd);
    sock_fd = -1;
  }
}

/// @brief program entry point
int main(int argc, char *argv[])
{
  char *host = argc > 1 ? argv[1] : DEFAULT_HOST;
  uint16_t port = argc > 2 ? atoi(argv[2]) : DEFAULT_PORT;

  if (port > 0xffff) syntax("Port must be in range 0-65535.", stx_str);

  // connect to host:port
  sock_fd = connect_to(host, port);
  atexit(close_connection);

  // run the telnet client
  run_client(sock_fd);

  // that's all, folks
  return EXIT_SUCCESS;
}
