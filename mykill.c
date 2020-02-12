#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char **argv)
{
  /* Juan driving
  * Check if user typed the right number of words. If wrong
  * input format exit of out program.
  */
  if(argc != 2) {
    printf("Input should be: <filename> <PID>\n");
    exit(1);
  }

  /* Get the second word (pid) and send SIGURS1 signal to 
  * process. If signal not sent, print error message and exit
  * program.
  */
  pid_t pid = (int) atoi(argv[1]); 
  
  if(kill(pid, SIGUSR1)) {
    printf("Kill error.\n");
    exit(1);
  } 
  return 0;
}
