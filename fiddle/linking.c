#include <stdio.h>
#include <stdlib.h>

int global_var = 1;

int main(){
  foo();
  global_var = 2;
  int local_var = 3;
  return 0;
}


