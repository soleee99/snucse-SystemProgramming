#include <unistd.h>

int main(void){
  char c;

  while(read(STDIN_FILENO, &c, 1) != 0){
    write(STDOUT_FILENO, &c, 1);
  }

}
