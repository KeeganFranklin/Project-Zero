#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"

/* Juan driving
* Print out message and prevent a termination that would happen
* under default behavior.
*
* Cited from the class site for Project 0.
*/
void sigHandler(int s) {
  ssize_t bytes;
  const int STDOUT = 1;
  bytes = write(STDOUT, "Nice Try.\n", 10);
  if(bytes != 10){
    exit(-999);
  }
}

/* Keegan driving
* Print out exiting message and exit signal handler
*
* Printing cited from the class site for Project 0.
*/
void sigExit() {
  ssize_t bytes;
  const int STDOUT = 1;
  bytes = write(STDOUT, "exiting.\n", 10);
  if(bytes != 10) {
    exit(-999);
  }
  exit(1);
}

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
  /* Keegan driving
  * Varaibles include the nanosecond equivalen of 1 second and a pid.
  */
  long numNanosInSec = 999999999;
  pid_t pid;

  /* Juan driving
  * Modify the action the program takes if it receives the signals
  * SIGINT and SIGURS1. In this case the action taken is our
  * user-made signal handlers.
  */
  Signal(SIGINT, sigHandler);
  Signal(SIGUSR1, sigExit);

  /* Keegan driving
  * Declare and instantiate values to keep track of time. 
  */
  struct timespec start, stop;
  start.tv_sec = 0; 
  start.tv_nsec = numNanosInSec;
  pid = (int) getpid();
  printf("%d\n", pid);

  /* Run infinite loop and print message every one second. */
  while (1) {
    printf("Still here\n"); 
    nanosleep(&start, &stop);
  }
  return 0;
}


