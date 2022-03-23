/* 
 * myspin.c - A handy program for testing your tiny shell 
 * 
 * usage: myspin <n>
 * Sleeps for <n> seconds in 1-second chunks.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) 
{
    //printf("myspin executed\n"); fflush(stdout);
    int i, secs;

    if (argc != 2) {
	fprintf(stderr, "Usage: %s <n>\n", argv[0]);
	exit(0);
    }
    secs = atoi(argv[1]);
    //printf("total secs: %d", secs);
    for (i=0; i < secs; i++){
	sleep(1);
  //printf("myspin: %d\n", i); fflush(stdout);
    }
    exit(0);
}
