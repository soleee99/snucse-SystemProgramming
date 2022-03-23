//--------------------------------------------------------------------------------------------------
// System Programming                         Shell Lab                                    Fall 2020
//
/// @author <Jinsol Park>
/// @studid <2018-14000>
//--------------------------------------------------------------------------------------------------

/*
 * unix shell : interactive command-line interpreter
 * command + command line argument
 * sheel executes built in commands in current process
 * shell forks a child process directed by pathname of an executable program
 * can either be run in the background or the foreground (using &)
 * job control / signal commands
 *
 * tsh> 
 * 1) handle io redirection
 * 2) handle signals
 * 3) handle back/fore ground (processid, job id) - JID denoted on command line by %
 * !! if all trace files work well it's fine!!
 * 4) support (muiltiple) pipes
 * 5) pipes and redirection together
 *
 * <<4 built-in commands need to be handled>>
 *  - quit
 *  - jobs : list all background jobs
 *  - bg <PID or JID>
 *  - fg <PID or JID>
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXPIPES      8   /* max MAXPIPES */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */
#define ERROR_ -1

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
  pid_t pid;              /* job PID */
  int jid;                /* job ID [1, 2, ...] */
  int state;              /* UNDEF, BG, FG, or ST */
  char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/*----------------------------------------------------------------------------
 * Functions that you will implement
 */

void eval(char *cmdline);
int builtin_cmd(char *(*argv)[MAXARGS]  );
void do_bgfg(char *(*argv)[MAXARGS]  );
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);

/*----------------------------------------------------------------------------*/

/* These functions are already implemented for your convenience */
int parseline(const char *cmdline, char *(*argv)[MAXARGS],  int* pipec);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);    
handler_t *Signal(int signum, handler_t *handler);

int redirect_in_koo(char **cmd);  // not exist
int redirect_out_koo(char **cmd);   // not exist


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
  // Redirect stderr outputs to stdout
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
    eval(cmdline);
    //printf("-------------------------\n");
    
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
 * When there is redirection(>), it return file name to char** file
 */


void eval(char *cmdline)
{  
  char *(argv[MAXLINE])[MAXARGS];  
  int rpipec;
  int bg = parseline(cmdline, argv, &rpipec);  // returns 1 if to run in background
  pid_t pid;
  sigset_t mask;

  if(!builtin_cmd(argv)){                     // when command is not bult in
    int pipefd[rpipec-1][2];                  // to keep pipe

    for(int i = 0 ; i < rpipec ; i++){
      sigemptyset(&mask);
      sigaddset(&mask, SIGCHLD);              // add SIGCHILD to block this signal
      sigprocmask(SIG_BLOCK, &mask, NULL);    // block signal before forking child
      
      if(i != rpipec-1) pipe(pipefd[i]);

      pid_t pid = fork();

      if(pid == 0){                           // TODO: if child
        setpgid(0, 0);                        // process id of calling group is used, make gid same as pid
        sigprocmask(SIG_UNBLOCK, &mask, NULL);  // unblock SIGCHLD signal

        /* ***********************************PIPE***********************************
         * if not the last command, then redirect STDOUT to the pipe write end
         * if no the first command, then redirect STDIN to the pipe read end
         * in each case, close the unused end,
         * call dup2,
         * then close the used end to decrease refcnt
         * ***********************************PIPE***********************************/
         
        if(i != rpipec-1){    // redirecting output
          close(pipefd[i][0]);
          dup2(pipefd[i][1], STDOUT_FILENO);
          close(pipefd[i][1]);
        }
        if(i != 0){           // redirecting input
          close(pipefd[i-1][1]);
          dup2(pipefd[i-1][0], STDIN_FILENO);
          close(pipefd[i-1][0]);
        }

        /* ***********************************REDIRECTION***********************************
         * first, find the position of ">" character by looping around 'while'
         * if the found position (dirsign below) is not 0, open a new file
         * the file name is the string after the ">" character
         * redirect STDOUT to the file
         * make the two last strings of the command as null
         *    - this is needed to later pass onto execvp propery
         * ***********************************REDIRECTION***********************************/
         
        int j = 0;
        int dirsign = 0;
        while(argv[i][j] != NULL){
          if(!(strcmp(argv[i][j], ">"))) dirsign = j;
          j++;
        }

        if(dirsign != 0){
          int redirect = open(argv[i][dirsign+1], O_RDWR|O_CREAT, S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP);
          dup2(redirect, 1);
          argv[i][j-1] = NULL;
          argv[i][j-2] = NULL;
        }

        /* ***********************************EXECUTION***********************************
         * call exevp with the command argv[i][0]
         * and arguments argv[i]
         * if return value is < 0, means it failed. print error msg
         * if errno is ENOENT, print error msg
         * ***********************************EXECUTION***********************************/
         
        if(execvp(argv[i][0], argv[i]) < 0){
          printf("failed to execute process\n");
          if(errno == ENOENT) printf(": No such file or directory\n");
          exit(EXIT_SUCCESS);
        }

      } else {                                    // TODO: when parent
        if(i != 0){                               // unless first command, need to close the previous pipes
          close(pipefd[i-1][0]);
          close(pipefd[i-1][1]);
        }
        if(!bg){                                  // if to run in foreground
          addjob(jobs, pid, FG, cmdline);
          sigprocmask(SIG_UNBLOCK, &mask, NULL);  // unblock SIGCHLD signal
          waitfg(pid);                            // parent waits for foreground job to terminate
        } else{                                   // if to run in background
          addjob(jobs, pid, BG, cmdline);
          sigprocmask(SIG_UNBLOCK, &mask, NULL);
          printf("[%d] (%d) %s", pid2jid(pid), (int)pid, cmdline);
        }
      }
    }
  }
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 * argv[MAXPIPES][MAXARGS]
*/


int parseline(const char *cmdline, char *(*argv)[MAXARGS] , int *rpipec  )
{
  static char array[MAXLINE]; /* holds local copy of command line */
  char* buf = array;          /* ptr that traverses command line */
  char* delim;                /* points to first space delimiter */
  char* pdelim;               /* points to pipe */
  int argc;                   /* number of args */
  int bg=0;                   /* background job? */
  int pipec;

  strcpy(buf, cmdline);
  buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */

  argc = 0;// How many argv
  pipec = 0;  // pipe count

  // ignore leading spaces
  while (*buf && (*buf == ' ')) buf++;

  if (*buf == '\'') {
    buf++;
    delim = strchr(buf, '\'');  // gets path
  } else {
    delim = strchr(buf, ' ');
  }
  // strchr : returns pointer to char if exists char in string, else return null

  while (delim) {
    argv[pipec ][ argc++] = buf;
    *delim = '\0';
    buf = delim + 1;

    // ignore spaces
    while (*buf && (*buf == ' ')) buf++;

    if (*buf) pdelim = strchr(buf, '|');

    // if there is pipe right on buf pointer
    if (*buf && pdelim  && *buf == *pdelim) {
      pipec++;
      argc=0;
      buf = buf + 1;

      // ignore spaces
      while (*buf && ( *buf == ' ')) buf++;
    }

    if (*buf == '\'') {
      buf++;
      delim = strchr(buf, '\'');
    } else {
      delim = strchr(buf, ' ');
    }
  }
  argv[pipec][ argc] = NULL;
  pipec++;
  *rpipec = pipec;// change pipec value for eval()

  // ignore blank line
  if (argc == 0) return 1;

  // should the job run in the background?
 if ((bg = (strcmp(argv[pipec-1][argc-1] , "&") == 0)) != 0) {
    argv[pipec-1][--argc] = NULL; 
  }
  return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */

int builtin_cmd(char *(*argv)[MAXARGS] )
{
  if(!strcmp(argv[0][0], "quit")){          // when command is 'quit', just exit
    exit(EXIT_SUCCESS);
  } else if (!strcmp(argv[0][0], "jobs")){  // when command is 'jobs', call listjobs
    listjobs(jobs);
    return 1;                               // returns 1 if builtin command is executed successfully
  } else if (!strcmp(argv[0][0], "bg") || !strcmp(argv[0][0], "fg")){ // when command is 'bg' or 'fg', call do_bgfg
    do_bgfg(argv); 
    return 1;                               // returns 1 if builtin command is executed successfully
  }
    
  return 0;                                 // 0 is returned when not builtin command
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */

void do_bgfg(char *(*argv)[MAXARGS] )
{
  pid_t pid;
  if(argv[0][1] == NULL) {                                                                  // when the command has no arguments
    if(!strcmp(argv[0][0], "fg")) printf("fg command requires PID of %%jobid argument\n");
    else printf("bg command requires PID or %%jobid argument\n");
    return;
  } else if(!(48 <= *argv[0][1] && *argv[0][1] <= 57) && !(*argv[0][1] == 37)){             // when command is not a number or does not begin with %
    if(!strcmp(argv[0][0], "bg")) printf("bg: argument must be a PID or %%jobid\n");
    else printf("fg: argument must be a PID or %%jobid\n");
    return;
  }

  // from here, it means proper inputs were given
  char* argument = argv[0][1];
  struct job_t *job = NULL;

  if(argument[0] == '%'){                         // if JID
    int jid = atoi(&argument[1]);                 // get JID as an integer
    job = getjobjid(jobs, jid);                   // get job by JID
    if(job == NULL){                              // means no such job exists
      printf("%s: No such job\n", argv[0][1]);
      return;
    } else {                                      // set pid to the pid of the job
      pid = job->pid;
    }
  } else{                                         // if PID
    pid = atoi(argument);                         // get PID as an integer
    job = getjobpid(jobs, pid);                   // get job by PID
    if(job == NULL){                              // means no such job exists
      printf("(%s): No such process\n", argv[0][1]);
      return;
    } 
  }

  if(!strcmp(argv[0][0], "bg")){                          // if command is 'bg'
    printf("[%d] (%d) %s", job->jid, pid, job->cmdline);
    job->state = BG;                                      // change state to BG
    kill(-pid, SIGCONT);                                  // send SIGCONT signal to whole group
  } else {                                                // if command is 'fg'
    job->state = FG;                                      // change state to FG
    kill(-pid, SIGCONT);                                  // send SIGCONT signal to whole group
    waitfg(job->pid);                                     // then, wait for the foreground job
  }


}

/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{ 
  struct job_t *job = getjobpid(jobs, pid);
  if(job != NULL){
    while(pid == fgpid(jobs)){}             // meaningless loop to check if the job with pid is in fg (busy)
  }
  return;                                   // return if done
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
  int status;
  pid_t pid;
  struct job_t *job;
  while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){   // reaping zombie child
    // wait set given as all of parent's child processed
    // WNOHANG | WUNTRACED : return 0 immediately if none of children in wait set has stopped or terminated
    // or return PID of one of stopped or terminated children

    if(WIFEXITED(status)){            // when terminated normally
      deletejob(jobs, pid);           // just delete the job
    } else if(WIFSIGNALED(status)){   // when terminated abnormally
      printf("Job [%d] (%d) terminated by signal 2\n", pid2jid(pid), (int)pid); fflush(stdout);
      deletejob(jobs, pid);
    } else if(WIFSTOPPED(status)){    // when stopped
      //printf("in WIFSTOPPED\n");
      printf("Job [%d] (%d) stopped by signal 20\n", pid2jid(pid), (int)pid); fflush(stdout);
      job = getjobpid(jobs, pid);
      job->state = ST;                // change state to ST
    }
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
  if(pid != 0){
    kill(-pid, sig);        // send SIGINT to entire foreground process group
  }
  return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
//
void sigtstp_handler(int sig)
{
  pid_t pid = fgpid(jobs);
  struct job_t *job;
  if(pid != 0){
    kill(-pid, sig);              // send signal to the whole group
    job = getjobpid(jobs, pid);
    job->state = ST;              // change state to ST
  }
  return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
  job->pid = 0;
  job->jid = 0;
  job->state = UNDEF;
  job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
  int i;

  for (i = 0; i < MAXJOBS; i++)
    clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs)
{
  int i, max=0;

  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].jid > max)
      max = jobs[i].jid;
  return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
  int i;
  if (pid < 1)
    return 0;
  //printf("next jid :%d \n", nextjid);
  for (i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == 0) {
      jobs[i].pid = pid;
      jobs[i].state = state;
      jobs[i].jid = nextjid++;
      if (nextjid > MAXJOBS)
        nextjid = 1;
      strcpy(jobs[i].cmdline, cmdline);
      if(verbose){
        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
      }
      return 1;
    }
  }
  printf("Tried to create too many jobs\n");
  return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid)
{
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == pid) {
      clearjob(&jobs[i]);
      nextjid = maxjid(jobs)+1;
      return 1;
    }
  }
  return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
  int i;

  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].state == FG)
      return jobs[i].pid;
  return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
  int i;

  if (pid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].pid == pid)
      return &jobs[i];
  return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid)
{
  int i;

  if (jid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].jid == jid)
      return &jobs[i];
  return NULL;  // returns NULL if no such job exists
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
  int i;

  if (pid < 1)
    return 0;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].pid == pid) {
      //printf("got right pid, jid is: %d\n", jobs[i].jid); fflush(stdout);
      return jobs[i].jid;
    }
  return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs)
{
  int i;

  for (i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid != 0) {
      printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
      switch (jobs[i].state) {
        case BG:
          printf("Running ");
          break;
        case FG:
          printf("Foreground ");
          break;
        case ST:
          printf("Stopped ");
          break;
        default:
          printf("listjobs: Internal error: job[%d].state=%d ",
              i, jobs[i].state);
      }
      printf("%s", jobs[i].cmdline);
    }
  }
}
/******************************
 * end job list helper routines
 ******************************/


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
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
  fprintf(stdout, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
  fprintf(stdout, "%s\n", msg);
  exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler)
{
  struct sigaction action, old_action;

  action.sa_handler = handler;
  sigemptyset(&action.sa_mask); /* block sigs of type being handled */
  action.sa_flags = SA_RESTART; /* restart syscalls if possible */

  if (sigaction(signum, &action, &old_action) < 0)
    unix_error("Signal error");
  return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
  printf("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}

/* $end tshref-ans */
