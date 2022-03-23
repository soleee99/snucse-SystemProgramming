#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#define READ 0
#define WRITE 1

/* Writing a program that
 *    - executes "ls -l /etc" in a child process
 *    - redirects output of that child process to the parent using a pipe
 *    - the parent reads from pipe, transforms and prints data to stdout
 * part of lecture note */

void child(int* pipefd){
  //close unused read end and redirect
  //standard out to write end of pipe
  printf("Child - close READ\n");
  close(pipefd[READ]);  // need to close unused ends
  printf("Child - redirect STDOUT to end of pipe\n");
  dup2(pipefd[WRITE], STDOUT_FILENO);
  // standard output is now redirected to pipe write
  
  char *argv[] = {
    "/bin/ls",
    "-l",
    "/etc",
    NULL,
  };
  printf("Child - execute\n");
  execv(argv[0], argv); // (pathname, arguments)
  // execv does not return when successful
  
  exit(EXIT_FAILURE);
}


void parent(int* pipefd){
  printf("Parent - close WRITE\n");
  close(pipefd[WRITE]); // need to close unused ends
  int even = 1;
  char c;
  while(read(pipefd[READ], &c, 1) > 0){
    if (even) c = toupper(c);
    else c = tolower(c);
    even = !even;
    write(STDOUT_FILENO, &c, sizeof(c));
  }
  exit(EXIT_SUCCESS);
}


int main(int arc, char* argv[]){
  int pipefd[2];
  pid_t pid;

  if(pipe(pipefd) < 0){
    printf("Unable to create pipe\n");
    exit(EXIT_FAILURE);
  }
  
  pid = fork();
  if(pid > 0) parent(pipefd);
  else if (pid == 0) child(pipefd);
  else printf("Cannot fork\n");

}

