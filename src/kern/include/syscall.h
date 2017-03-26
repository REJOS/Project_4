#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <machine/trapframe.h>

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys_getpid(pid_t *retval);
int sys_fork(struct trapframe *tf, pid_t *retval);
void sys__exit(int code);
int sys_execv(const char *prog, char *const *args);
int sys_waitpid(pid_t pid, int *returncode, int flags);

#endif /* _SYSCALL_H_ */
