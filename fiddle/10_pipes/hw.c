#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>

#define R 0
#define W 1

int main() {
  int fd[2];
  pid_t p;

  if(pipe(fd) < 0) return EXIT_FAILURE;

  if((p=fork()) >0) {
    printf("Parent: sending message to child...\n");
    char write_msg[] = "Hello";

    close(fd[R]);
    for(int i = 0 ; i < strlen(write_msg) ; i++){
      write(fd[W], &write_msg[i], 1);
    }
    close(fd[W]);
    printf("parent exits.\n");
  } else {
    printf("Child: receiving message from aprent...\n");
    char c;
    close(fd[W]);
    while(read(fd[R], &c, 1) > 0) {
      printf("%c\n", c);
    }
    close(fd[R]);
    printf("Child exits.\n");
  }

  return EXIT_SUCCESS;
}
