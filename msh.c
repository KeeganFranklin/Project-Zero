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
    // Juan driving
    int isBG, isCommand;
    char *argv[MAXARGS];
    pid_t pid;
    sigset_t mask;

    isBG = parseline(cmdline, argv);
    if(argv[0] == NULL) { // Error checking if no input
        return;
    }
    isCommand = builtin_cmd(argv); 
    if (!isCommand) {
        // sigprocmask start 
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        pid = fork(); 
        if (pid < 0) {
            unix_error("fork error");
        }

        // **************error checking if pid is not working

        if(pid == 0) {
            setpgid(0, 0);
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            if (execve(argv[0], argv, environ) < 0) { // B&O page 791
                printf("%s: Command not found\n", argv[0]); 
            }

            // error check if execve is janky, command not found
        } else {
            if (!isBG) {
                if(addjob(jobs, pid, FG, cmdline)) {
                    sigprocmask(SIG_UNBLOCK, &mask, NULL);
                    waitfg(pid); 
                } else {
                    // do something
                    kill(-pid, SIGINT);
                    // error check for kill, use unix_error here
                }
            } else {
                if(addjob(jobs, pid, BG, cmdline)) {
                    printf("[%d] (%d) %s", pid2jid(jobs, pid), pid, cmdline);
                    sigprocmask(SIG_UNBLOCK, &mask, NULL);
                } else {
                    // do something
                    kill(-pid, SIGINT);
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
    char* command = argv[0];
    if (!strcmp(command, "quit")) {
        exit(0);
    } else if(!strcmp(argv[0], "jobs")) {
	    listjobs(jobs);
	    return 1;
    } else if(!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg" )) {
        if(argv[1] == NULL) {
            printf("%s command requires PID or %%jobid argument\n", argv[0]);//write?
        } else {
            do_bgfg(argv); 
        }
        return 1; 
    }
    return 0;     /* not a builtin command */
}

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
    struct job_t *jobby;
    int jid, pidNum, isNum;
    char first = argv[1][0];

    // if(argv[1] == NULL) {
    //     printf("%s command requires PID or %%jobid argument\n", argv[0]);//write?
    //     return;
    // }

    // Check if argv[1] is a number
    isNum = first == '%' ? !isNumber(argv[1], 1) : !isNumber(argv[1], 0);
    if(isNum) {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);// write
        return;
    }

    if(first == '%') {

        jid = atoi(&argv[1][1]); // Put in & because we kept getting error of a pointer in terminal
        jobby = getjobjid(jobs, jid);
        if(jobby == NULL) {
            //unix_error("job does not exist\n"); -> fix later
            printf("%s: No such job\n", argv[1]);
            return;
        }

    } else { // sent in a process id
        pidNum = atoi(argv[1]);
        jobby = getjobpid(jobs, pidNum);
        if(jobby == NULL) {
            printf("(%s): No such process\n", argv[1]);
            return;
        }
    }

    kill(-jobby->pid, SIGCONT); // stupid ass name, just make it sendSignal

    if(!strcmp(argv[0], "bg")) {
        jobby->state = BG;
        printf("[%d] (%d) %s", jobby->jid, jobby->pid, jobby->cmdline);
    } else { // bg command
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
    sigset_t mask, prev;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    sigprocmask(SIG_BLOCK, &mask, &prev);

    while(fgpid(jobs) != 0) {
        sigsuspend(&prev);
    }

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
    pid_t pid;
    int status;
    struct job_t *jobby;
    char str[100];
    const int STDOUT = 1;
    
    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {

        jobby = getjobpid(jobs, pid);

        if(WIFSIGNALED(status)) {
            sprintf(str, "Job [%d] (%d) terminated by signal %d\n", jobby->jid, pid, WTERMSIG(status));
            if(write(STDOUT, str, strlen(str)) != strlen(str)) {
                exit(-999);
            }

        } else if(WIFSTOPPED(status)) {
            sprintf(str, "Job [%d] (%d) stopped by signal %d\n", jobby->jid, pid, WSTOPSIG(status));
            if(write(STDOUT, str, strlen(str)) != strlen(str)) { // on error, write returns -1
                exit(-999);
            }
            jobby->state = ST;
            return;
        }
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
    pid_t pid = fgpid(jobs);
    if (pid) {
        kill(-pid, sig); 
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
    pid_t pid = fgpid(jobs);
    if (pid) {
        kill(-pid, sig); 
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
