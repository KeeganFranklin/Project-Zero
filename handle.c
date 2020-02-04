#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"


// Juan driving
void sigHandler(int s) {

  // Cited from class site Project 0
  ssize_t bytes;
  const int STDOUT = 1;
  bytes = write(STDOUT, "Nice Try.\n", 10);
  if(bytes != 10){
    exit(-999);
  }
}

void sigExit() {
  const int STDOUT = 1;
  write(STDOUT, "exiting.\n", 10);
  exit(1);
}
// Juan stops driving

/*
 * First, print out the process ID of this process.
 *
 * Then, set up the signal handler so that ^C causes
 * the program to print "Nice try.\n" and continue looping.
 *
 * Finally, loop forever, printing "Still here\n" once every
 * second.
 */
int main(int argc, char **argv)
{
  // Keegan drives
  long numNanosInSec = 999999999;
  pid_t pid;
  // Keegan stop 

  // Juan drives
  Signal(SIGINT, sigHandler);
  Signal(SIGUSR1, sigExit);
  // Juan stops

  // Keegan drives
  struct timespec start, stop;
  start.tv_sec = 0; 
  start.tv_nsec = numNanosInSec;
  pid = (int) getpid(); // might change
  printf("%d\n", pid); 
  while (1) {
    printf("Still here\n"); 
    nanosleep(&start, &stop);

  }
  return 0;
  // Keegan stops
}


