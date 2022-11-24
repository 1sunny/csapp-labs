/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include "defs.h"

extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */

char *args[MAXARGS];        /* buf for cmd args */
int verbose = 0;
/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) {
  char c;
  char cmdline[MAXLINE];
  int emit_prompt = 1; /* emit prompt (default) */

  /* Redirect stderr to stdout (so that driver will get all output
   * on the pipe connected to stdout) */
  // dup2(1, 2);

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
  Signal(SIGINT, sigint_handler);   /* ctrl-c */
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
      P("%s", prompt);
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
 * the foreground, wait for it to terminate and then return.
 * Note: each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) {
  D("\n---------\nget cmd: %s\n", cmdline);
  int bg = parseline(cmdline, args);
  int builtin = builtin_cmd(args);
  if (builtin)
    return;

  sigset_t mask_chld, prev;
  sigaddset(&mask_chld, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask_chld, &prev);

  pid_t pid;
  if ((pid = fork()) == 0) { /* child */
    /* shell is running in the foreground process group.
     * If your shell then creates a child process,
     * by default that child will also be a member of the foreground process group.
     * Since typing ctrl-c sends a SIGINT to every process in the foreground group,
     * typing ctrl-c will send a SIGINT to your shell,
     * as well as to every process that your shell created */
    /* pgid = 0,就用pid指向的进程的PID作为进程组ID,将子进程与shell分离
     * parent还是shell
     * */
    if (setpgid(0, 0) < 0) {
      unix_error("setpgid");
    }
    sigprocmask(SIG_UNBLOCK, &mask_chld, &prev);
    /* 成功则函数不会返回,执行失败则直接返回-1,失败原因存于errno 中 */
    if (execve(args[0], args, environ) < 0) {
      unix_error(args[0]);
      return;
    }
  }
  D("pid: %d\n", pid);
  addjob(jobs, pid, bg ? BG : FG, cmdline);
  sigprocmask(SIG_UNBLOCK, &mask_chld, &prev);
  if (bg == 0) {
    waitfg(pid);
  } else {
    P("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
  }
  D("\ndrop cmd: %s\n-----------", cmdline);
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.
 * Return true if the user has requested a BG job,
 * false if the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv) {
  static char array[MAXLINE]; /* holds local copy of command line */
  char *buf = array;          /* ptr that traverses command line */
  char *delim;                /* points to first space delimiter */
  int argc;                   /* number of args */
  int bg;                     /* background job? */

  strcpy(buf, cmdline);
  buf[strlen(buf) - 1] = ' ';  /* replace trailing '\n' with space */
  while (*buf && (*buf == ' ')) /* ignore leading spaces */
    buf++;

  /* Build the argv list */
  argc = 0;
  if (*buf == '\'') {
    buf++;
    delim = strchr(buf, '\'');
  } else {
    delim = strchr(buf, ' ');
  }

  while (delim) {
    argv[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
    while (*buf && (*buf == ' ')) /* ignore spaces */
      buf++;

    if (*buf == '\'') {
      buf++;
      delim = strchr(buf, '\'');
    } else {
      delim = strchr(buf, ' ');
    }
  }
  argv[argc] = NULL;

  if (argc == 0)  /* ignore blank line */
    return 1;

  /* should the job run in the background? */
  if ((bg = (*argv[argc - 1] == '&')) != 0) {
    argv[--argc] = NULL;
  }
  return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) {
  char *cmd = argv[0];
  if (strcmp(cmd, "quit") == 0) {
    exit(0);
  } else if (strcmp(cmd, "jobs") == 0) {
    listjobs(jobs);
    return 1;
  } else if (strcmp(cmd, "bg") == 0 || strcmp(cmd, "fg") == 0) {
    do_bgfg(argv);
    return 1;
  } else if (strcmp(cmd, "&") == 0 || strcmp(cmd, "") == 0) {
    return 1;
  }
  return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) {
  D("in %s\n", __func__);
  if(argv[1] == NULL || strcmp(argv[1], "") == 0){
    P("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }
  int is_job = (argv[1][0] == '%');
  int id = Atoi(argv[1] + is_job);
  if(id < 0){
    P("%s: argument must be a PID or %%jobid\n", argv[0]);
    return;
  }
  struct job_t *job = is_job ? getjobjid(jobs, id) : getjobpid(jobs, id);
  if (job == NULL) {
    if(is_job){
      P("%s: No such job\n", argv[1]);
    }else{
      P("(%s): No such process\n" , argv[1]);
    }
    return;
  }
  D("do_bgfg id: %d\n", id);
  kill(-job->pid, SIGCONT);
  if (strcmp(argv[0], "fg") == 0) {
    jobs->state = FG;
    waitfg(job->pid);
  } else {
    job->state = BG;
    P("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
  }
  D("out %s\n", __func__);
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid) {
  D("in %s\n", __func__);
  sigset_t mask_none;
  sigemptyset(&mask_none);
  while (fgpid(jobs) > 0) {
    /**
     * 将进程的信号屏蔽字设置为由sigmask指向的值
     * 在捕捉到一个信号或发生了一个会终止该进程的信号之前,该进程被挂起
     * 如果捕捉到一个信号而且从该信号处理程序返回,则sigsuspend返回,
     * 并且将该进程的信号屏蔽字设置为调用sigsuspend之前的值
     * always return -1 with errno = EINTR
     */
    sigsuspend(&mask_none);
  }
  D("out %s\n", __func__);
}

/*
 * usage - print a help message
 */
void usage(void) {
  printf("Usage: shell [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

