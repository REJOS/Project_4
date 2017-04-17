#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <machine/trapframe.h>
#include <types.h>

/*
 * argvdata struct
 * temporary storage for argv, global and synchronized.  this is because
 * a large number of simultaneous execv's could bring the system to its knees
 * with a huge number of kmallocs (and even reasonable sized command lines
 * might not fit on the stack).
 */
struct argvdata {
	char *buffer;
	char *bufend;
	size_t *offsets;
	int nargs;
	struct lock *lock;
};




/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */
void init_argdata();
int sys_reboot(int code);
int sys_getpid(pid_t *retval);
int sys_fork(struct trapframe *tf, pid_t *retval);
void sys__exit(int code);
int sys_execv(userptr_t prog, userptr_t argv);
int sys_waitpid(pid_t pid, int *returncode, int flags);

int sys_open(const char *path, int oflag);
int sys_read(int fd, void *buf, size_t nbytes);
int sys_write(int fd, const void *buf, size_t nbytes);
int sys_lseek(int fd, off_t offset, int whence);
int sys_close(int fd);
int sys_dup2(int oldfd, int newfd);

#endif /* _SYSCALL_H_ */
