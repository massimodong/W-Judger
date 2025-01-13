#include "jail.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <seccomp.h>
#include <signal.h>
#include "syscall_whitelist.h"

static void _wjudger_exit(int s, siginfo_t *info, void *ucontext){
  while (1);
  fprintf(stderr, "Not allowed system call: %d\n", info->si_syscall);
  fflush(stderr);
  exit(-1);
}

static inline int error(){
  while(1);
}

#define WJUDGER_SYSCALL_ALLOW(s) (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(s), 0) ? error() : 1)

static void apply_seccomp(){
  scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_TRAP);
  if(ctx == NULL) error();

  SYSCALL_ALLOWED_ALL(WJUDGER_SYSCALL_ALLOW);

  int load_res = seccomp_load(ctx);
  if(load_res) error();
  seccomp_release(ctx);
}

[[ noreturn ]] static void spawned(const char *exe){
  const char *Main[] = { exe, NULL };

  jail();
  apply_seccomp();

  struct sigaction new_action;
  new_action.sa_sigaction = _wjudger_exit;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = SA_SIGINFO;

  if(sigaction(SIGSYS, &new_action, NULL)){
    exit(-1);
  }

  execvp(Main[0], (char * const *)Main);
  while(1);
}

int main(int argc, char *argv[]){
  if(argc != 3) return -1;
  const char *exe = argv[1];
  int status_fd;
  if(sscanf(argv[2], "%d", &status_fd) != 1) return -1;

  pid_t pid = fork();
  if(pid == -1) return -1;

  if(pid == 0){
    while(close(status_fd) == -1);
    spawned(exe);
  }else{
    int status;
    struct rusage usage;
    if(wait4(pid, &status, 0, &usage) == -1) return -1;
    if(write(status_fd, &status, sizeof(status)) != sizeof(status)) return -1;
    if(write(status_fd, &usage, sizeof(usage)) != sizeof(usage)) return -1;
    return 0;
  }
}
