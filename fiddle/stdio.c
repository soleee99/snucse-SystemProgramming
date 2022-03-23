#include <stdio.h>
#include <stdlib.h>
extern FILE *stdin;
extern FILE *stdout;

int main(){
  FILE *f = fopen("./input.txt", "r");
  char* c = malloc(7);

  fgets(c,7, stdin);
  printf("%s\n", c);
}
