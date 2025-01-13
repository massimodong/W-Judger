#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/socket.h>

#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/audit.h>

#include <signal.h>

#include "syscall_whitelist.h"

#define ArchField offsetof(struct seccomp_data, arch)

#define Deny(syscall) \
    BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_##syscall, 0, 1), \
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRAP)

static struct sock_filter filter[] = {
    /* validate arch */
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, ArchField),
    BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, AUDIT_ARCH_X86_64, 1, 0),
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRAP),

    /* load syscall */
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, offsetof(struct seccomp_data, nr)),

    /* list of denied syscalls */
	SYSCALL_ALLOWED_EXTENDED(Deny),

    /* and if we don't match above, ok */
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
};
static struct sock_fprog filterprog = {
    .len = sizeof(filter)/sizeof(filter[0]),
    .filter = filter
};

static void _wjudger_exit(int s, siginfo_t *info, void *ucontext){
	fprintf(stderr, "Not allowed system call: %d\n", info->si_syscall);
	exit(-1);
}

__attribute__((constructor(0))) static void _wjudger_load_seccomp(){
	struct sigaction new_action;
	new_action.sa_sigaction = _wjudger_exit;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = SA_SIGINFO;
	
	if(sigaction(SIGSYS, &new_action, NULL)){
		exit(-1);
	}

	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        exit(-1);
    }
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &filterprog)) {
        exit(-1);
    }
}