#include <stdio.h>
#include <unistd.h>
#define READ 0
#define WRITE 1

// Program to write and read two messages using pipe

int main(){
  int pipefd[2];  // pipe file descriptors, 0 for read, 1 for write
  int rtstatus;   // return status
  char wrmsg[2][20] = {"Hi", "Hello"};
  char rdmsg[20];

  rtstatus = pipe(pipefd);
  /* the above system call creates pipe for one-way communication
   * creates two descriptoes
   *    - first one to read from pipe
   *    - second one to write into pipe */

  if (rtstatus == -1){
    printf("Unable to create pipe\n");
    return 1;
  }

  printf("Writing to pipe - Message 1 is %s\n", wrmsg[0]);
  write(pipefd[WRITE], wrmsg[0], sizeof(wrmsg[0])); // (int fd, coid* buf, size_t cnt)

  read(pipefd[READ], rdmsg, sizeof(rdmsg)); // (int fd, void* buf, size_t cnt)
  printf("Reading from pipe - Message 1 is %s\n", rdmsg);

  printf("Writing to pipe - Message 2 is %s\n", wrmsg[1]);
  write(pipefd[WRITE], wrmsg[1], sizeof(wrmsg[1]));

  read(pipefd[READ], rdmsg, sizeof(rdmsg));
  printf("Reading from pipe - Message 2 is %s\n", rdmsg);
  return 0;
}

