#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char* argv[]){
  pid_t pid[N];
  int i, chid_status;

  for(i = 0 ; i < N ; i++){
    if((pid[i] = fork()) == 0){
      // child
}
