#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys_getpid(pid_t *retval);
void _exit(int code);
int execv(const char *prog, char *const *args);
pid_t fork(void);
int waitpid(pid_t pid, int *returncode, int flags);

#endif /* _SYSCALL_H_ */
