//--------------------------------------------------------------------------------------------------
// System Programming                          Network Lab                                 Fall 2020
//
/// @file
/// @brief Simple virtual McDonald's server for Network Lab
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
/// TODO: Code modified & added by Jinsol Park (2018-14000) - fin
//--------------------------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include "net.h"

/// @name Macro Definitions
/// @{
#define PORT 7777                                            /// default port number
#define BUF_SIZE 1024                                        /// default send & recv buffer size
#define CUSTOMER_MAX 20                                      /// number of maximum clients to handle
#define NUM_KITCHEN 5                                        /// number of kitchen thread(s)
/// @}
//
#define LOCK(x) if (pthread_mutex_lock(x) != 0) fprintf(stderr, "ERROR: mutex error");
#define UNLOCK(x) if (pthread_mutex_unlock(x) != 0) fprintf(stderr, "ERROR: mutex error");

enum burger_type {
    BURGER_BIGMAC,
    BURGER_CHEESE,
    BURGER_CHICKEN,
    BURGER_BULGOGI,
    BURGER_TYPE_MAX
};

//static pthread_mutex_t t_mtx = PTHREAD_MUTEX_INITIALIZER;
/// @name Structures
/// @{

/// @brief general node element to implement a singly-linked list
typedef struct __node {
    struct __node *next;                                      ///< pointer to next node
    unsigned int customerID;                                  ///< customer ID that requested
    enum burger_type type;                                    ///< requested burger type
    bool is_ready;                                            ///< true if burger is ready
    pthread_cond_t cond;                                      ///< conditional variable
    pthread_mutex_t mutex;                                    ///< mutex variable
} Node;

/// @brief order data
typedef struct __order_list {
    Node *head;                                               ///< head of order list
    Node *tail;                                               ///< tail of order list
    unsigned int count;                                       ///< number of nodes in list
} OrderList;

/// @brief structure for server context
struct mcdonalds_ctx {
    unsigned int total_customers;                             ///< number of customers served
    unsigned int total_burgers[BURGER_TYPE_MAX];              ///< number of burgers produced by types
    unsigned int total_queueing;                               ///< number of customers in queue
    OrderList list;                                           ///< starting point of list structure
};

/// @}

/// @name Global Variables
/// @{
int listenfd;                                                 ///< listen file descriptor
struct mcdonalds_ctx server_ctx;                              ///< keeps server context
sig_atomic_t keep_running = 1;                                ///< keeps all the threads running
pthread_t kitchen_thread[NUM_KITCHEN];                        ///< thread for kitchen
char *burger_names[] = {"bigmac", "cheese", "chicken", "bulgogi"}; ///< structure to store burger names
/// @}

/// @brief Enqueue element in tail of the OrderList
/// @param customerID customer ID
/// @param type burger type
/// @retval Node* containing the node structure of the element
Node* issue_order(unsigned int customerID, enum burger_type type) 
{   // the enqueue function

    Node *new_node = malloc(sizeof(Node));

    new_node->customerID = customerID;
    new_node->type = type;
    new_node->next = NULL;
    new_node->is_ready = false;
    pthread_cond_init(&new_node->cond, NULL);
    pthread_mutex_init(&new_node->mutex, NULL);
    if (server_ctx.list.tail == NULL) {           // if list is empty, initialize
        server_ctx.list.head = new_node;
        server_ctx.list.tail = new_node;
    }
    else {                                        // if list has a node already, put new node at end
        server_ctx.list.tail->next = new_node;
        server_ctx.list.tail = new_node;
    }
    server_ctx.list.count++;                      // increase number of nodes in OrderList

    return new_node;                              // returns the newly added node
}


/// @brief Dequeue element from the OrderList
/// @retval Node* Node from head of the list
Node* get_order(void) 
{   // the deqeueue function
    Node *target_node;

    if (server_ctx.list.head == NULL) {   // if the list is empty, just return NULL
        return NULL;
    }

    target_node = server_ctx.list.head;

    if (server_ctx.list.head == server_ctx.list.tail) {
        server_ctx.list.head = NULL;
        server_ctx.list.tail = NULL;
    }
    else {
        server_ctx.list.head = server_ctx.list.head->next;
    }

    server_ctx.list.count--;

    return target_node;
}

/// @brief Returns number of element left in OrderList
/// @retval number of element(s) in OrderList
unsigned int order_left(void)
{
    int ret;

    ret = server_ctx.list.count;

    return ret;
}

/// @brief Kitchen task for kitchen thread
void* kitchen_task(void *dummy)
{
    Node *order;
    enum burger_type type;
    pthread_t tid = pthread_self();

    printf("Kitchen thread %lu ready\n", tid);

    while (keep_running || order_left()) {  // if press ctr+c, keep_running = false
        order = get_order();
        if (order == NULL) {
            sleep(2);
            continue;
        }

        type = order->type; // burger type
        printf("[Thread %lu] generating %s burger\n", tid, burger_names[type]);
        sleep(5);
        printf("[Thread %lu] %s burger is ready\n", tid, burger_names[type]);

        server_ctx.total_burgers[type]++;

        order->is_ready = true;
        pthread_cond_signal(&order->cond);  // signal client thread to wake up(?)
    }

    printf("[Thread %lu] terminated\n", tid);
    pthread_exit(NULL);
}

/// @brief client task for client thread
/// @param newsock socketID of the client as void*
void* serve_client(void* fd)
{
    int clientfd = *((int*)fd);             // cast the void pointer to int
    ssize_t read, sent;
    size_t msglen;
    char *message, *burger, *buffer;
    unsigned int customerID;
    enum burger_type type;
    Node *order = NULL;
    int ret, i;                             // ret is length of string

    buffer = (char *) malloc(BUF_SIZE);
    msglen = BUF_SIZE;

    customerID = server_ctx.total_customers++;

    printf("Customer #%d visited\n", customerID);

    // send welcome to mcdonalds
    ret = asprintf(&message, "Welcome to McDonald's, customer #%d\n", customerID);  // generate string to client
    if (ret < 0) {
        perror("asprintf");
        server_ctx.total_queueing--;                                                // unable to serve unexpectedly. decrease total_ququeing before return
        return NULL;
    }

    sent = put_line(clientfd, message, ret);                                        // fd, buffer, length of string, send a line
    if (sent < 0) {
        printf("Error: cannot send data to client\n");
        goto err;
    }
    free(message);

    // receive order from the customer
    read = get_line(clientfd, &buffer , &msglen); // read line from the client, placed in buffer,
                                                  // 'read' is total bymber of read bytes

    // parse order from the customer
    burger = buffer;
    burger[read-1] = '\0';                        // add a null character at ent

    // if burger is not available, exit connection
    int burger_exist = 0;                         // exist flag 0: flase, 1: true
    for(int k = 0 ; k < 4 ; k++){
      if(strcmp(burger_names[k], burger)==0){     // if 'burger_names[k]' and 'burger' is equal
        burger_exist = 1;                         // change the flag
        i = k;                                    // assign the index to i, to be used later
      }
    }
    if(!burger_exist){                            // when requested burger does not exist in burger_names
      fprintf(stderr, "ERROR: burger not available\n");
      server_ctx.total_queueing--;                // decrease from total_queueing before return
      close(clientfd);
      return NULL;
    }
    
    // issue order to kitchen and wait
    if(strcmp("bulgogi", burger) == 0) type = BURGER_BULGOGI;
    if(strcmp("chicken", burger) == 0) type = BURGER_CHICKEN;
    if(strcmp("cheese", burger) == 0) type = BURGER_CHEESE;
    if(strcmp("bigmac", burger) == 0) type = BURGER_BIGMAC;     // set thee type to the requested burger

    order = issue_order(customerID, type);                      // issue order
    pthread_cond_wait(&order->cond, &order->mutex);             // wait for the kitchen thread to finish

    // if order successfully handled, hand burger and say goodbye
    if (order->is_ready) {
        ret = asprintf(&message, "Your %s burger is ready! Goodbye!\n", burger_names[i]);
        sent = put_line(clientfd, message, ret);
        if (sent <= 0) {
            printf("Error: cannot send data to client\n");
            goto err;
        }
        free(message);
    }

    free(order);

err:
    server_ctx.total_queueing--;    // customer is served, or error -> decrease number of customers
    close(clientfd);
    free(buffer);
    return NULL;
}


/// @brief start server listening
void start_server()
{
    server_ctx.total_queueing = 0;

    int addrlen, opt = 1;
    struct sockaddr_in client;
    struct addrinfo *ai, *ai_it;

    // open port
    ai = getsocklist(NULL, PORT, AF_UNSPEC, SOCK_STREAM, 1, NULL);                  // ai is list of addrinfo

    // iterate through potential addressinfo structs
    ai_it = ai;
    while(ai_it != NULL){
      listenfd = socket(ai_it->ai_family, ai_it->ai_socktype, ai_it->ai_protocol);  // 'socket' returns the file descriptor of listening socket
      if(listenfd != -1){
        if((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(int)) == 0) &&
            (bind(listenfd, ai_it->ai_addr, ai_it->ai_addrlen) == 0) &&
            (listen(listenfd, 32) == 0)) break;                                     // if we get a proper listenfd, then break
        //close(fd);
      }
      ai_it = ai_it->ai_next;                                                       // if not, check the next addressinfo struct
    }

    if(ai_it == NULL) perror("ERROR: Cannot bind to port.");
    freeaddrinfo(ai);
    addrlen = ai_it->ai_addrlen;

    printf("Listening...\n");
    pthread_t tid;

    while (1) {
        int* clientfd = malloc(sizeof(int));                                                // take clientfd as a pointer to prevent race condition
        *clientfd = accept(listenfd, (struct sockaddr *) &client, (socklen_t *) &addrlen);  // listen file descriptor
        if (clientfd < 0) {
            perror("accept");
            continue;
        }else if (server_ctx.total_queueing < CUSTOMER_MAX){                                // if there is queueing space left
          server_ctx.total_queueing++;                                                      // increase the number of people in the queue
          //fprintf(stderr, "number of queueing customers: %d\n", server_ctx.total_queueing); fflush(stdout); 
          pthread_create(&tid, NULL, serve_client, (void*)clientfd);                        // create a thread that runs serve_client 
        }else{
          printf("Max number of customers exceeded, Good bye!\n"); fflush(stdout);          // more than CUSTOMER_MAX clients
          close(*clientfd);                                                                 // close the connection
        }

    }
}

/// @brief prints overall statistics
void print_statistics(void)
{
    int i;

    printf("\n====== Statistics ======\n");
    printf("Number of customers visited: %u\n", server_ctx.total_customers);
    for (i = 0; i < BURGER_TYPE_MAX; i++) {
        printf("Number of %s burger made: %u\n", burger_names[i], server_ctx.total_burgers[i]);
    }
    printf("\n");
}

/// @brief exit function
void exit_mcdonalds(void)
{
    close(listenfd);
    print_statistics();
}

/// @brief Second SIGINT handler function
/// @param sig signal number
void sigint_handler2(int sig)
{
    exit_mcdonalds();
    exit(EXIT_SUCCESS);
}

/// @brief First SIGINT handler function
/// @param sig signal number
void sigint_handler(int sig)
{
    signal(SIGINT, sigint_handler2);
    printf("****** I'm tired, closing McDonald's ******\n");
    keep_running = 0;
}

/// @brief init function initializes necessary variables and sets SIGINT handler
void init_mcdonalds(void)
{
    int i;

    printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@@@@(,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,(@@@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@@,,,,,,,,,,,,,,,,,@@@@@@@@@@@@@@@@@,,,,,,,,,,,,,,,,,@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@,,,,,,,@@@@@@,,,,,,,@@@@@@@@@@@@@@(,,,,,,@@@@@@@,,,,,,,@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@,,,,,,@@@@@@@@@@,,,,,,,@@@@@@@@@@@,,,,,,,@@@@@@@@@*,,,,,,@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@.,,,,,,@@@@@@@@@@@@,,,,,,,@@@@@@@@@,,,,,,,@@@@@@@@@@@@,,,,,,/@@@@@@@@@@\n");
    printf("@@@@@@@@@,,,,,,,,@@@@@@@@@@@@@,,,,,,,@@@@@@@,,,,,,,@@@@@@@@@@@@@,,,,,,,,@@@@@@@@@\n");
    printf("@@@@@@@@,,,,,,,,@@@@@@@@@@@@@@@,,,,,,,@@@@@,,,,,,,@@@@@@@@@@@@@@@,,,,,,,,@@@@@@@@\n");
    printf("@@@@@@@@,,,,,,,@@@@@@@@@@@@@@@@,,,,,,,,@@@,,,,,,,,@@@@@@@@@@@@@@@@,,,,,,,@@@@@@@@\n");
    printf("@@@@@@@,,,,,,,,@@@@@@@@@@@@@@@@@,,,,,,,,@,,,,,,,,@@@@@@@@@@@@@@@@@,,,,,,,,@@@@@@@\n");
    printf("@@@@@@,,,,,,,,@@@@@@@@@@@@@@@@@@,,,,,,,,,,,,,,,,,@@@@@@@@@@@@@@@@@@,,,,,,,,@@@@@@\n");
    printf("@@@@@,,,,,,,,,@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@,,,,,,,,,@@@@@\n");
    printf("@@@@@,,,,,,,,@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@,,,,,,,,@@@@@\n");
    printf("@@@@,,,,,,,,,@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,@@@@\n");
    printf("@@@@,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,@@@@\n");
    printf("@@@,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,@@@\n");
    printf("@@@,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,@@@\n");
    printf("@@@,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,@@@\n");
    printf("@@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@@\n");
    printf("@@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@@\n");
    printf("@@,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,@@\n");
    printf("@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@\n");
    printf("@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@\n");
    printf("@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@\n");
    printf("@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@\n");
    printf("@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@\n");
    printf("@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@\n");
    printf("@,,,,,,,,,,@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@,,,,,,,,,,@\n");

    printf("\n\n                          I'm lovin it! McDonald's                          \n");

    signal(SIGINT, sigint_handler);

    server_ctx.total_customers = 0;
    server_ctx.total_queueing = 0;
    for (i = 0; i < BURGER_TYPE_MAX; i++) {
        server_ctx.total_burgers[i] = 0;
    }


    for(int i = 0 ; i < NUM_KITCHEN ; i++){
      pthread_create(&kitchen_thread[i], NULL, kitchen_task, NULL);
    }

    for(int i = 0 ; i < NUM_KITCHEN ; i++){
      pthread_detach(kitchen_thread[i]);
    }
}

/// @brief program entry point
int main(int argc, char *argv[])
{
    init_mcdonalds();
    start_server();
    exit_mcdonalds();

    return 0;
}
