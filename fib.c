#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

const int MAX = 13;

static void doFib(int n, int doPrint);


/*
 * unix_error - unix-style error routine.
 */
inline static void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}


int main(int argc, char **argv)
{
    int arg;
    int print=1;

    if(argc != 2){
        fprintf(stderr, "Usage: fib <num>\n");
        exit(-1);
    }

    arg = atoi(argv[1]);
    if(arg < 0 || arg > MAX){
        fprintf(stderr, "number must be between 0 and %d\n", MAX);
        exit(-1);
    }

    doFib(arg, print);

    return 0;
}

/* 
 * Recursively compute the specified number. If print is
 * true, print it. Otherwise, provide it to my parent process.
 *
 * NOTE: The solution must be recursive and it must fork
 * a new child for each call. Each process should call
 * doFib() exactly once.
 */
static void doFib(int n, int doPrint)
{
    // Juan Nava driving
    pid_t pid1;
    pid_t pid2;
    int total = 0;

    int status; //you can just have this i guess

    if(n == 0) {
        if(doPrint) {
            printf("0\n"); // Not sure if need to get out yet since it's a base case
        }
        exit(0); // This makes the exit status equal 0
    } else if(n == 1) {
        if(doPrint) {
            printf("0\n");
        }
        exit(1); // This makes the exit status equal 1
    } else {
        // Focus on call with n - 1
        pid1 = fork();

        // If we are in this child process call the function again
        if(pid1 == 0) {
            // I'm sure we're only supposed to print once
            doFib(n - 1, 0);
        }

        // Focus on call with n - 2
        pid2 = fork();

        // If we are in this child process call the function again
        if(pid2 == 0) {
            doFib(n - 2, 0);
        }

        /* None of the if statements were entered so we are in the
        parent process
        
        The parent has to wait for the child process to finish
        so we can get its exit status
        */
        waitpid(pid1, &status, 0);

        /* Once we know a child process has terminated properly
        We can get the exit status of that child and it to 
        this version of total
        */
        if(WIFEXITED(status)) {
            total += WEXITSTATUS(status);
        }

        waitpid(pid2, &status, 0);

        if(WIFEXITED(status)) {
            total += WEXITSTATUS(status);
        }

        if(doPrint) {
            printf("%d\n", total);// Again not sure if need to get out yet since it's a base case
        }
        exit(total); // return this as the exit status for the parent

        // Juan Nava has stopped driving

    }
    
  
}


