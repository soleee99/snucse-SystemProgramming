//--------------------------------------------------------------------------------------------------
// System Programming                          Network Lab                                 Fall 2020
//
/// @file
/// @brief Client-side implementation of Network Lab
/// @author Hyunik Kim <hyunik@csap.snu.ac.kr>
/// @section changelog Change Log
/// 2020/11/18 Hyunik Kim created
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "net.h"

/// @name Macro Definitions
/// @{
#define PORT 7777                                            /// default port number
#define BUF_SIZE 1024                                        /// default send & recv buffer size
/// @}
enum burger_type {
    BURGER_BIGMAC,
    BURGER_CHEESE,
    BURGER_CHICKEN,
    BURGER_BULGOGI,
    BURGER_TYPE_MAX
};

/// @name Global Variable
/// @{
char *burger_names[] ={"bigmac", "cheese", "chicken", "bulgogi"}; ///< structure to store burger names
/// @}

/// @brief client task for connection thread
void *thread_task(void *data)
{
    struct addrinfo *ai, *ai_it;
    size_t read, sent, buflen;
    int serverfd = -1, choice, ret;
    char *buffer;
    pthread_t tid;

    tid = pthread_self();

    buffer = (char *)malloc(BUF_SIZE);
    buflen = BUF_SIZE;

    // Connect to McDonald's server
    ai = getsocklist("127.0.0.1", PORT, AF_UNSPEC, SOCK_STREAM, 0, NULL);

    ai_it = ai;
    while (ai_it != NULL) {
        serverfd = socket(ai_it->ai_family, ai_it->ai_socktype, ai_it->ai_protocol);
        if (serverfd != -1) {
            if (connect(serverfd, ai_it->ai_addr, ai_it->ai_addrlen) != -1) break;
            close(serverfd);
        }
        ai_it = ai_it->ai_next;
    }

    if (ai_it == NULL) {
        perror("connect");
        goto err;
    }

    // Read welcome message from the server
    read = get_line(serverfd, &buffer, &buflen);
    if (read <= 0) {
        printf("Cannot read data from server\n");
        goto err;
    }

    printf("[Thread %lu] From server: %s", tid, buffer);

    // Randomly choose which burger to order
    choice = rand() % BURGER_TYPE_MAX;
    ret = strlen(burger_names[choice]);
    printf("[Thread %lu] To server: I want %s burger!\n", tid, burger_names[choice]);

    memset(buffer, 0, BUF_SIZE);
    strncpy(buffer, burger_names[choice], ret);
    buffer[ret] = '\n';

    // Send burger name to the server
    sent = put_line(serverfd, buffer, strlen(buffer));
    if (sent < 0) {
        printf("Error: cannot send data to client\n");
        goto err;
    }

    // Get final message from the server
    memset(buffer, 0, BUF_SIZE);
    read = get_line(serverfd, &buffer, &buflen);
    if (read <= 0) {
        printf("Cannot read data from server\n");
        goto err;
    }

    printf("[Thread %lu] From server: %s", tid, buffer);

err:
    close(serverfd);
    freeaddrinfo(ai);
    pthread_exit(NULL);
}

/// @brief program entry point
int main(int argc, char const *argv[])
{
    int i;
    int num_threads;

    if (argc != 2) {
        printf("usage ./client <num_threads>\n");
        return 0;
    }

    num_threads = atoi(argv[1]);
    srand(time(NULL));

    pthread_t *thread_arr = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);
    for (i = 0; i < num_threads; i++) {
        pthread_create(&thread_arr[i], NULL, thread_task, NULL);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(thread_arr[i], NULL);
    }
    
    return 0;
}
