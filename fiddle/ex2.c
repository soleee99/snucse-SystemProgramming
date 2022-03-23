#include <stdio.h>
#include <stdlib.h>

int main(int agc, char* argv[]){
  int i, x, y;
  int c = atoi(argv[1]);
  x = 1;
  y = 2;
  printf("%d\n", c);
  for(i = 0 ; i < 5 ; i++){
    x += y;
    y++;
  }
  return 0;
}
