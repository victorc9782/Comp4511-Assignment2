#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXARGS 128
#define MAXLINE 8192

extern char **environ; /* defined by libc */
/* function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int main()
{
    char cmdline[MAXLINE]; /* Command line */

    while (1)
    {
	    printf("> "); /* Read */
	    fgets(cmdline, MAXLINE, stdin);
	    if (feof(stdin))
        {
	        exit(0);
        }
	    eval(cmdline); /* Evaluate */
    }
}

/* Evaluate a command line */
void eval(char *cmdline)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
    {
	    return;   /* Ignore empty lines */
    }

    if (!builtin_command(argv))
    {
	    if ((pid = fork()) == 0) /* Child runs user job */
        {
            if (execve(argv[0], argv, environ) < 0)
            {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
        /* Parent waits for foreground job to terminate */
        if (!bg)
        {
            int status;
            if (waitpid(pid, &status, 0) < 0)
            {
                fprintf(stderr, "waitfg: waitpid error: %s\n",
                        strerror(errno));
            }
        }
        else
        {
            printf("%d %s", pid, cmdline);
        }
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "exit")) /* exit command */
    {
        exit(0);
    }
    if (!strcmp(argv[0], "&"))
    {
        return 1;
    }
    return 0; /* Not a builtin command */
}

/* Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{

    return bg;
}


