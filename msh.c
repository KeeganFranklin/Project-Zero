/* 
 * msh - A mini shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "util.h"
#include "jobs.h"


/* Global variables */
int verbose = 0;            /* if true, print additional output */

extern char **environ;      /* defined in libc */
static char prompt[] = "msh> ";    /* command line prompt (DO NOT CHANGE) */
static struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
void usage(void);
void sigquit_handler(int sig);



/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    /* Juan driving */
    int isBG, isCommand;
    char *argv[MAXARGS];
    pid_t pid;
    sigset_t mask;

    /* Call parseline to change words of input into argv and save
    * return value into isBG to know first word is a BG job. */
    isBG = parseline(cmdline, argv);

    /* Return back to shell if no input was detected (just Enter). */
    if(argv[0] == NULL) {
        return;
    }

    /* Call builtin_cmd function to check if first word is a built in
    * command and perform the fucntion. Otherwise enter if_statement.
    */
    isCommand = builtin_cmd(argv); 
    if (!isCommand) {

        /* Empty mask variable that holds blocked signals and add
        * SIGCHLD to mask. Then block SIGCHLD with sigprocmask.
        */ 
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        /* Keegan driving
        * Use fork and create a child process, while error checking
        * if fork failed.
        */
        pid = fork(); 
        if (pid < 0) {
            unix_error("fork error");
        }

        /* If in the child process, execute program. */
        if(pid == 0) {

            /* Change the process group of child as it will ensure only
            * one process is in the foreground process group. Also
            * unblock the SIGCHLD.
            */
            setpgid(0, 0);
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            /* Execute the program in the pathname (first word), and print
            * error messsage if execve fails and return to top of shell.
            * 
            * Cited from B&O pg. 791
            */
            if (execve(argv[0], argv, environ) < 0) { // B&O page 791
                printf("%s: Command not found\n", argv[0]); 
                exit(1);
            }

        /* Enter parent process */
        } else {

            /* If we have a foreground job and are able to add the job,
            * then unblock SIGCHLD and wait for the job to finish
            */
            if(!isBG && addjob(jobs, pid, FG, cmdline)) {
                sigprocmask(SIG_UNBLOCK, &mask, NULL);
                waitfg(pid);

            /* If we have a background job and are able to add the job,
            * then print the job info, and unblock sIGCHLD.
            */
            } else if(isBG && addjob(jobs, pid, BG, cmdline)) {
                printf("[%d] (%d) %s", pid2jid(jobs, pid), pid, cmdline);
                sigprocmask(SIG_UNBLOCK, &mask, NULL);

            /* If no job was able to be added because list is full or
            * the pid was below 0, then kill process.
            */
            } else {
                if (kill(-pid, SIGINT) < 0) {
                    unix_error("kill error");
                }
            }
        }
    }
    return;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * Return 1 if a builtin command was executed; return 0
 * if the argument passed in is *not* a builtin command.
 */
int builtin_cmd(char **argv) 
{
    /* Juan driving
    * Get the first word which may be a built-in command and execute it.
    * If the first word is "quit", then exit the shell.
    */
    char* command = argv[0];
    if (!strcmp(command, "quit")) {
        exit(0);

    /* Command to list the jobs. */
    } else if(!strcmp(argv[0], "jobs")) {
	    listjobs(jobs);
	    return 1;

    /* Keegan driving
    * Check if the first word is "bg" or "fg".
    */
    } else if(!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg" )) {

        /* Check if there is second word at all in the array, printing
        * an error message if there is no second word. If that passes,
        * then go to the do_bgfg function.
        */
        if(argv[1] == NULL) {
            printf("%s command requires PID or %%jobid argument\n", argv[0]);
        } else {
            do_bgfg(argv); 
        }
        return 1; 
    }
    return 0;     /* not a builtin command */
}

/* Keegan driving
* This is a helper function that checks if a particular string is 
* a number. It is used to make sure fg/bg command line argument
* can be processed.
*/
int isNumber(char* str, int startIndex) {
    int index = startIndex; 
    int letterFound = 0; 
    while (!letterFound && str[index]) {
        letterFound = !isdigit(str[index]);
        index++;
    }
    return !letterFound; 
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    /* Keegan driving */
    struct job_t *jobby;
    int jid, pidNum, isNum;
    char first = argv[1][0];

    /* Check if the second word is a number. */
    isNum = first == '%' ? !isNumber(argv[1], 1) : !isNumber(argv[1], 0);

    /* Error checking, argument after fg/bg command has to be a number */
    if(isNum) {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }

    /* If argument following command has a %, it is a jobid and the job
    * struct must be acquired. If job isn't found then print error
    * message and exit.
    */
    if(first == '%') {

        jid = atoi(&argv[1][1]);
        jobby = getjobjid(jobs, jid);
        if(jobby == NULL) {
            printf("%s: No such job\n", argv[1]);
            return;
        }

    /* Juan driving
    * Get the job struct with the PID. If the job doesn't exist, print error
    * message and return to the top of the shell.
    */
    } else {
        pidNum = atoi(argv[1]);
        jobby = getjobpid(jobs, pidNum);
        if(jobby == NULL) {
            printf("(%s): No such process\n", argv[1]);
            return;
        }
    }

    /* Restart a stopped job by sending the SIGCONT signal. */
    if (kill(-jobby->pid, SIGCONT) < 0) {
        unix_error("kill error");
    }

    /* Once a job was restarted, if built in command was "bg", then change 
    * the state to BG and print message. Otherwise change the state to
    * FG and wait for this new foreground job.
    */
    if(!strcmp(argv[0], "bg")) {
        jobby->state = BG;
        printf("[%d] (%d) %s", jobby->jid, jobby->pid, jobby->cmdline);
    } else {
        jobby->state = FG;
        waitfg(jobby->pid);
    }

    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    /* Juan driving
    * Variables describe a set containing signals to be blocked
    */
    sigset_t mask, prev;

    /* Empty the mask set and and add SIGCHLD as a signal to be
    * blocked. Finally block the signal with sigprocmask.
    */
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &prev);

    /* Continuously run loop until foreground process is not the
    * parameter pid. Use sigsuspend to avoid busy waiting and race
    * conditions by suspending process until child terminates and
    * avoid being interrupted between while check and sigsuspend.
    */
    while(fgpid(jobs) == pid) {
        sigsuspend(&prev);
    }

    /* Unblock SIG_CHLD after child already terminated. */
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    /* Juan driving 
    * Influenced by eval function in B&O pg. 809
    */
    pid_t pid;
    int status;
    struct job_t *jobby;
    char str[100];
    const int STDOUT = 1;
     
    /* This while loop continues until any child process has changed
    * its state. If a child has changed its state to stop, the program
    * changes its state in jobs and exits to the command line. Otherwise
    * the program continues to wait and to delete zombie children.
    */
    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {

        jobby = getjobpid(jobs, pid);

        /* If pid is a process that has terminated, then print message out
        * and delete job.
        */
        if(WIFSIGNALED(status)) {
            sprintf(str, "Job [%d] (%d) terminated by signal %d\n", 
                jobby->jid, pid, WTERMSIG(status));
            if(write(STDOUT, str, strlen(str)) != strlen(str)) {
                exit(-999);
            }

        /* If pid is a process that stopped, then print message out,
        * change status, and return to top of shell loop.
        */
        } else if(WIFSTOPPED(status)) {
            sprintf(str, "Job [%d] (%d) stopped by signal %d\n", 
                jobby->jid, pid, WSTOPSIG(status));
            if(write(STDOUT, str, strlen(str)) != strlen(str)) {
                exit(-999);
            }
            jobby->state = ST;
            return;
        }

        /* Delete jobs that have been terminated. */
        deletejob(jobs, pid);
    }
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    /* Keegan driving 
    * Get the pid of the foreground job and send the SIGINT signal
    */
    pid_t pid = fgpid(jobs);
    if (pid) {
        if (kill(-pid, sig) < 0) {
            unix_error("kill error");
        }
    }
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    /* Juan driving 
    * Get the pid of the foreground job and send the SIGTSTP signal
    * if a foreground job exists.
    */
    pid_t pid = fgpid(jobs);
    if(pid) {
        if (kill(-pid, sig) < 0) {
            unix_error("kill error");
        }
    }
    return;
}

/*********************
 * End signal handlers
 *********************/



/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    ssize_t bytes;
    const int STDOUT = 1;
    bytes = write(STDOUT, "Terminating after receipt of SIGQUIT signal\n", 45);
    if(bytes != 45)
       exit(-999);
    exit(1);
}