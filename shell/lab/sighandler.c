#include "defs.h"

int ret;         /* syscall return value */
/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) {
  struct sigaction action, old_action;

  action.sa_handler = handler;
  sigemptyset(&action.sa_mask); /* block sigs of type being handled */
  action.sa_flags = SA_RESTART; /* restart syscalls if possible */

  if (sigaction(signum, &action, &old_action) < 0)
    unix_error("Signal error");
  return (old_action.sa_handler);
}
/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 * 1.子进程终止时会向父进程发送SIGCHLD信号,该信号的默认处理动作为忽略
 * 2.子进程接收到SIGSTOP(19)信号停止时
 * 3.子进程处在停止态,接受到SIGCONT后唤醒时
 */
void sigchld_handler(int sig) {
  D("in %s\n", __func__);
  int olderrno = errno;
  sigset_t mask_all, prev;
  pid_t pid;
  sigfillset(&mask_all);
  sigprocmask(SIG_SETMASK, &mask_all, &prev);
  int status;
  // wait for all child
  while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
    D("current waitpid: %d, ", pid);
    if (WIFSTOPPED(status)) {
      D("stopped\n");
      struct job_t *job = getjobpid(jobs, pid);
      job->state = ST;
      P("Job [%d] (%d) stopped by signal %d\n", job->jid, job->pid, WSTOPSIG(status));
    } else if (WIFEXITED(status)) {
      D("exit\n");
      deletejob(jobs, pid);
    } else if (WIFSIGNALED(status)) {
      D("signaled\n");
      struct job_t *job = getjobpid(jobs, pid);
      P("Job [%d] (%d) terminated by signal %d\n", job->jid, job->pid, WTERMSIG(status));
      deletejob(jobs, pid);
    } else if (WIFCONTINUED(status)) {
      D("continued\n");
    } else {
      unix_error("waitpid unknown state");
    }
  }
  sigprocmask(SIG_SETMASK, &prev, NULL);
  errno = olderrno;
  D("out %s\n", __func__);
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig) {
  D("in %s\n", __func__);
  int olderrno = errno;
  sigset_t mask_int, prev;
  sigemptyset(&mask_int);
  sigaddset(&mask_int, SIGINT);
  sigprocmask(SIG_SETMASK, &mask_int, &prev);
  pid_t pid = fgpid(jobs);
  D("fgpid: %d\n", pid);
  if (pid) {
    ret = kill(-pid, SIGINT);
    if (ret == -1) {
      unix_error("kill");
    }
    D("kill ret: %d\n", ret);
    waitfg(pid);
  }
  sigprocmask(SIG_SETMASK, &prev, NULL);
  errno = olderrno;
  D("out %s\n", __func__);
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig) {
  D("in %s\n", __func__);
  int olderrno = errno;
  sigset_t mask_tstp, prev;
  sigemptyset(&mask_tstp);
  sigaddset(&mask_tstp, SIGTSTP);
  sigprocmask(SIG_SETMASK, &mask_tstp, &prev);
  pid_t pid = fgpid(jobs);
  D("fgpid: %d\n", pid);
  if (pid) {
    ret = kill(-pid, SIGTSTP);
    if (ret == -1) {
      unix_error("kill");
    }
    D("kill ret: %d\n", ret);
    waitfg(pid);
  }
  sigprocmask(SIG_SETMASK, &prev, NULL);
  errno = olderrno;
  D("out %s\n", __func__);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) {
  P("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}
