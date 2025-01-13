#ifndef _SYSCALL_WHITELIST_H_
#define _SYSCALL_WHITELIST_H_

#include <sys/syscall.h>

#define SYSCALL_ALLOWED(ACTION)\
	ACTION(exit_group),\
	ACTION(brk),\
	ACTION(mmap),\
	ACTION(munmap),\
	ACTION(write),\
	ACTION(read),\
	ACTION(fstat),\
	ACTION(lseek),\
	ACTION(newfstatat)

#define SYSCALL_ALLOWED_EXTENDED(ACTION)\
	ACTION(execve),\
	ACTION(arch_prctl),\
	ACTION(prctl),\
	ACTION(rt_sigaction),\
	ACTION(readlink),\
	ACTION(mprotect),\
	ACTION(arch_prctl),\
	ACTION(uname),\
	ACTION(ioctl),\
	ACTION(getrlimit),\
	ACTION(setrlimit),\
	ACTION(set_tid_address),\
	ACTION(set_robust_list),\
	ACTION(rseq),\
	ACTION(prlimit64),\
	ACTION(getrandom)

#define SYSCALL_ALLOWED_ALL(ACTION)\
	SYSCALL_ALLOWED(ACTION),\
	SYSCALL_ALLOWED_EXTENDED(ACTION)

#endif