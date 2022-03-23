#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define READ 0
#define WRITE 1

// Program to read and write two  messages through the pipe 
// using parent and child processes

int main(){
  int pipefd[2];
  int rtstatus;
  int pid;
  char wrmsg[2][20] = {"Hi","Hello"};
  char rdmsg[20];

  rtstatus = pipe(pipefd);
  if(rtstatus == -1){
    printf("Unable to create pipe\n");
    return 1;
  }
  
  pid = fork();
  // both parent anc child have copies of file descriptors to read/write end

  if(pid == 0){ // child process
    printf("Start child process\n");
    read(pipefd[READ], rdmsg, sizeof(rdmsg));
    printf("Child Process - Reading from pipe - Message 1 is %s\n", rdmsg);
    read(pipefd[READ], rdmsg, sizeof(rdmsg));
    printf("Child Process - Reading from pipe - Message 2 is %s\n", rdmsg);
    printf("End child process\n");
    close(pipefd[READ]);
  } else {  // parent process
    printf("Start parent process\n");
    printf("Parent Process - Writing to pipe - Message 1 is %s\n", wrmsg[0]);
    write(pipefd[WRITE], wrmsg[0], sizeof(wrmsg[0]));
    printf("Parent Process - Writing to pipe, Message 2 is %s\n", wrmsg[1]);
    write(pipefd[WRITE], wrmsg[1], sizeof(wrmsg[1]));
    printf("End parent process\n");
    close(pipefd[WRITE]);
  }

 exit(EXIT_SUCCESS);
}

