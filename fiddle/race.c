#include <stdio.h>
#include <stdlib.h>


int main(){
  int i = 0;
  while(i < 10){
    int tmp = i;
    printf("%p\n", &tmp);
    i++;
  }
  return 0;
}
