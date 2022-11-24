#ifndef LAB_DEFS_H
#define LAB_DEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

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

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */

extern int verbose;            /* if true, print additional output */

/* End global variables */

/* 在linux系统里面,一般都是行刷新,也就是要输出的内容会先放在缓冲区里面,
 * 直到遇到换行符,会将缓冲区里的内容全部输出到屏幕或者文件中 */
#define P(...) printf(__VA_ARGS__);fflush(stdout)

#define DE 1
#if DE == 1
#define D(...) fprintf(stderr, __VA_ARGS__);fflush(stdout)
#else
#define D(...)
#endif

/* Function prototypes */
/* tsh.c */
void eval(char *cmdline);

int parseline(const char *cmdline, char **argv);

int builtin_cmd(char **argv);

void do_bgfg(char **argv);

void waitfg(pid_t pid);

void usage(void);

/* sighandler.c */
void sigchld_handler(int sig);

void sigtstp_handler(int sig);

void sigint_handler(int sig);

void sigquit_handler(int sig);

typedef void handler_t(int);

handler_t *Signal(int signum, handler_t *handler);

/* job.c */
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

/* util.c */
int Atoi(const char* s);

void unix_error(char *msg);

void app_error(char *msg);
#endif //LAB_DEFS_H
