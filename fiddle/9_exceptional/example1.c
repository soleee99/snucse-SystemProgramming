#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

void handler2(int sig){
  pid_t pid;
  while((pid = waitpid(-1, NULL, 0)) > 0){
    printf("handler reaped child %d\n", (int)pid);
  }
  if (errno != ECHILD) printf("waitpid error");
  sleep(2);
  return;
}

int main(){
  int i, n;
  char buf[20];

  if(signal(SIGCHILD, handler2) == SIG_ERR){  // handle SIGCHILD signal with handler2
    printf("signal error\n");
  }

  for(i = 0 ; i < 3 ; i++){
    if(fork() == 0){  // if this is child process
      printf("hello from child %d\n", (int) getpid());
      sleep(1);
      exit(EXIT_SUCCESS);
    }
  }

  while((n = read(STDIN_FILENO, buf, sizeof(buf))) < 0){
    if(errno != EINTR) printf("signal error\n");
  }
  while(1);
}



