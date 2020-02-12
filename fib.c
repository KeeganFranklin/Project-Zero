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
    /* Juan driving
    * Variables for processes, keeping track of status, and total (sum).
    */
    pid_t pid1, pid2;
    int status, total = 0;

    /* Base case where n = 0
    * Print the value if doPrint is non-zero
    * Exit function with exit value being 0
    */
    if(n == 0) {
        if(doPrint) {
            printf("0\n");
        }
        exit(0);

    /* Base case where n = 1
    * Print the value if doPrint is non-zero
    * Exit function with exit value being 1
    */
    } else if(n == 1) {
        if(doPrint) {
            printf("0\n");
        }
        exit(1);

    /* Keegan driving
    * If none of the base cases are true create two processes with fork.
    * After forking two processes and waiting for them to finish, get
    * the exit status of each and add it to total.
    */
    } else {
        /* Fork the first child to call doFib for n - 1 and if
        * we are in this child process call the function again.
        */
        pid1 = fork();

        if(pid1 == 0) {
            doFib(n - 1, 0);
        }

        /* Fork the second child to call doFib for n - 2 and if
        * we are in this child process call the function again.
        */
        pid2 = fork();

        if(pid2 == 0) {
            doFib(n - 2, 0);
        }

        /* Since we have passed the if-statements, we are in the parent
        * process. The parent process will to have wait for each child
        * to finish and then record their exit status.
        * 
        * First wait for child 1
        */
        waitpid(pid1, &status, 0);

        /* Once a child process has terminated properly we can get the
        * exit status of that child and add it to this version of total.
        */
        if(WIFEXITED(status)) {
            total += WEXITSTATUS(status);
        }

        /* Waiting for the second child and get the exit status to add
        * to the parent process' version of total.
        */
        waitpid(pid2, &status, 0);

        if(WIFEXITED(status)) {
            total += WEXITSTATUS(status);
        }

        /* Juan driving
        * Print the total that was aqcuired from the child processes
        * and the potential other processes the children created if
        * doPrint is non-zero. Finally exit the function with the
        * exit status being the total of this parent process.
        */
        if(doPrint) {
            printf("%d\n", total);
        }
        exit(total);
    }
}


