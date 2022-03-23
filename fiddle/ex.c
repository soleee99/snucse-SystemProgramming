#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
  int x = 1;
  int *a;
  a = &x;

  pid_t pid = fork();
  if(pid == 0) { // child
    printf("Hi, I'm jinsol. Rin's daengdaneg\n");
    printf("Value of x : %d, value of a(add to x) : %p, value pointed by a: %d\n", ++x, a, *a);
  } else { // parent
    printf("hi, I'm seorkin. Jinsol's majesty\n");
    printf("Value of add of x : %d, value of a(add to x) : %p, value pointed by a: %d\n", --x, a, *a);
  }
  return 0;

}
