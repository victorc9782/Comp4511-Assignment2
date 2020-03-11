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
int interupted = 0;
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void interuptHandler(int signal);

int main()
{
    char cmdline[MAXLINE] = ""; /* Command line */
    signal(SIGINT, interuptHandler); 

    while (1)
    {
        char currentWholeDir[MAXLINE];
        char currentDir[MAXLINE];
        interupted = 0;
        if (getcwd(currentWholeDir, sizeof(currentWholeDir)) != NULL) {
            if (strcmp(currentWholeDir, "/") == 0){
                strcpy(currentDir, "/");
            }
            else if (strcmp(currentWholeDir,getenv("HOME")) == 0){
                strcpy(currentDir, "~");
            }
            else{
                //strcpy(currentDir, currentWholeDir);
                char *buf = strtok(currentWholeDir, "/"); 
                while (buf!=NULL){
                    //printf("%s\n", buf);
                    strcpy(currentDir, buf);
                    buf = strtok(NULL, "/"); 
                }
            }
        } 
        else{
            perror("getcwd() error");
            return 1;
        }
	    printf("[%s]> ", currentDir); /* Read */
        fgets(cmdline, MAXLINE, stdin);
        if (strcmp(cmdline,"\n")==0){
            continue;
        }
        if (!interupted){
            if (feof(stdin))
            {
                exit(0);
            }
            eval(cmdline); /* Evaluate */
        }
    }
}

void interuptHandler(int signal){
    printf("\nUse exit\n");
    interupted = 1;
}
/* Evaluate a command line */
void eval(char *cmdline)
{
    char **argv = (char**)malloc(MAXARGS*sizeof(char*)); /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    if (cmdline == NULL){
        return;
    }
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
    {
	    return;   /* Ignore empty lines */
    }
    int buildCommandResult = builtin_command(argv);
    if (!buildCommandResult)
    {
	    if ((pid = fork()) == 0) /* Child runs user job */
        {            
            int errnum;
            //if (execve(argv[0], argv, environ) < 0)
            if (execvp(argv[0], argv) < 0)
            {
                //errnum = errno;
                //fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));
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
    else{
        if (buildCommandResult == 2){
            if (argv[1] != NULL){
                chdir(argv[1]);
            }
            else{
                char homeDir[MAXLINE] = "";
                strcpy(homeDir,getenv("HOME"));
                if (homeDir==""){
                    perror("No Home Defined in Env\n");
                    exit(-1);
                }
                else{
                    chdir(homeDir);
                }
            }
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
    if (!strcmp(argv[0], "cd"))
    {
        return 2;
    }
    return 0; /* Not a builtin command */
}

/* Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
    int bg = 0;
    int count = 0;
    char *argvBuf = strtok(buf, "\n"); 
    argvBuf = strtok(argvBuf, " \t"); 
    while (argvBuf!=NULL && count<MAXARGS ){
        argv[count] = (char*)malloc(MAXLINE*sizeof(char));
        strcpy(argv[count], argvBuf);
        //printf("cmd1's buf: %s\n", buf);
        count++;
        argvBuf = strtok(NULL, " \t"); 
    }
    if (strcmp(argv[count-1], "&") == 0){
        printf("Run Background\n");
        argv[count-1] = (char*)NULL;
        bg = 1;
    }
    //execvp(argv[0], argv);
    return bg;
}


