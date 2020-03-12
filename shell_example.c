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
#define MAXCHILD 256

extern char **environ; /* defined by libc */
/* function prototypes */
int interupted = 0;
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void handler(int signal);
int addChildProcess(pid_t pid,char *cmdline );
int removeChildProcess(pid_t pid);
void printChildProcess();
typedef struct{
   pid_t pid;
   char cmdline[MAXLINE];
} childProcess ;
childProcess childProcessList[MAXCHILD];
int childProcessCounter = 0;
childProcess childRemoveList[MAXCHILD];
int childRemoveCounter = 0;

int main()
{
    char cmdline[MAXLINE] = ""; /* Command line */
    signal(SIGINT, handler); 
    signal(SIGCHLD, handler); 

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

void handler(int signal){
    int childPid = 0;
    switch(signal){
        case SIGINT:
            printf("\n");
            printf("Use exit\n");
            interupted = 1;
            break;
        case SIGCHLD:
            while( childPid= waitpid(-1, NULL, WNOHANG), childPid > 0){
                //printf("Collected Child Signal: %d", childPid);
                removeChildProcess(childPid);
            }
            break;
    }
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
            if (bg){
                int fd = open("/dev/null", O_WRONLY);
                dup2(fd, 1);    /* make stdout a copy of fd (> /dev/null) */
                dup2(fd, 2);    /* ...and same with stderr */
                close(fd);  
            }
            if (execvp(argv[0], argv) < 0)
            {
                //errnum = errno;
                //fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
        if (!bg)
        {
            /* Parent waits for foreground job to terminate */
            int status;
            if (waitpid(pid, &status, 0) < 0)
            {
                fprintf(stderr, "waitfg: waitpid error: %s\n",
                        strerror(errno));
            }
        }
        else
        {
            //printf("%d %s", pid, cmdline);
            int addProcessError = addChildProcess(pid, cmdline);
            if (!addProcessError){
                printf("Error in adding process\n");
            }
        }
    }
    else{
        if (!bg){
            if (buildCommandResult == -1){
                exit(0);
            }
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
            else if (buildCommandResult == 3){
                printChildProcess();
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
        return -1;
    }
    if (!strcmp(argv[0], "&"))
    {
        return 1;
    }
    if (!strcmp(argv[0], "cd"))
    {
        return 2;
    }
    if  (!strcmp(argv[0], "jobs"))
    {
        return 3;
    }
    /*
    if  (!strcmp(argv[0], "debug"))
    {
        return 4;
    }
    */
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
        argv[count-1] = (char*)NULL;
        bg = 1;
    }
    //execvp(argv[0], argv);
    return bg;
}

int addChildProcess(pid_t pid,char *cmdline ){
    if (childProcessCounter>=(MAXCHILD-1)){
        printf("Cannot child process handler full.\n");
        return -1;
    }
    childProcessList[childProcessCounter].pid = pid;
    strcpy(childProcessList[childProcessCounter].cmdline, cmdline);
    //printf("Added %d %s",childProcessList[childProcessCounter].pid, childProcessList[childProcessCounter].cmdline);
    childProcessCounter++;
}
int removeChildProcess(pid_t pid){
    int i = 0;
    int found = 0;
    //printf ("Target: %d\n", pid);
    while (i<childProcessCounter && !found){
        //printf("Searching [%d] %d %s", i, childProcessList[i].pid, childProcessList[i].cmdline);
        if (childProcessList[i].pid == pid){
            /*
            printf("Found removeChildProcess %d\n",pid);
            //DEBUG
            childRemoveList[childRemoveCounter].pid = childProcessList[i].pid;
            strcpy(childRemoveList[childRemoveCounter].cmdline, childProcessList[i].cmdline);
            childRemoveCounter++;
            //DEBUG END
            */

            int j = i;
            while (j<(childProcessCounter-1)){
                childProcessList[j].pid = childProcessList[j+1].pid;
                strcpy(childProcessList[j].cmdline, childProcessList[j+1].cmdline);
                j++;
            }
            childProcessList[childProcessCounter-1].pid = 0;
            strcpy(childProcessList[childProcessCounter-1].cmdline, "");
            childProcessCounter--;
            found = 1;
        }
        i++;
    }
    return found;
}
void printChildProcess(){
    if (childProcessCounter < 1){
        printf("No background process is running\n");
        return;
    }
    int i = 0;
    while (i<childProcessCounter){
        printf("[%d] %d %s", i, childProcessList[i].pid,  childProcessList[i].cmdline);
        i++;
    }
    return;
}
