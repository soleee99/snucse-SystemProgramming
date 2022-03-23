#include <stdio.h>
#include <stdlib.h>

int sum (int a, int b){
  return a+b;
}

int main(){
  int (*funcptr)(int, int);
  funcptr = sum;
  printf("%d\n", (int)funcptr(1, 3));
}


