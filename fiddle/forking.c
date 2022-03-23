#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(){
  pid_t pid;
  int x = 1;

  pid = fork();
  if(pid ==0){
    printf("child: x=%d, return value of fork: %d, my pid: %5d, parent pid: %5d\n", x, pid, getpid(), getppid());
    x = 2;
    printf("%d\n",x);
    exit(0);
    }
  printf("parent: x=%d, return value of fore : %d, my pid: %5d, parent pid: %5d\n", x, pid, getpid(), getppid());
  x = 3;
  exit(0);
}
