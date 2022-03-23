#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void read_bytes(int fd, int n){
  char c;
  while((n == -1) || (n-- > 0)){
      if(read(fd, &c, 1) == 1) printf("[%5d] %c, in read_bytes\Kn", getpid(), c);
      else break;
      sleep(2);
  }
}

int main(){
  int fd = STDIN_FILENO;
  read_bytes(fd, 3);

  pid_t pid = fork();
  if(pid >0) sleep(1);
  read_bytes(fd, -1);

  return EXIT_SUCCESS;
}
