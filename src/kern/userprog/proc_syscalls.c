#include <types.h>
#include <lib.h>
#include <syscall.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <kern/errno.h>
#include <../arch/mips/include/trapframe.h>



void _exit(int code) {



}


int execv(const char *prog, char *const *args) {


	return 0;
}


int sys_fork(struct trapframe *tf, pid_t *retval) {

	int result;
	struct thread * new_thread;

	struct trapframe * new_tf = kmalloc(sizeof(struct trapframe));
	if (new_tf == NULL) {
		return ENOMEM;
	}
	*new_tf = *tf;

	struct addrspace * new_addr;
	result = as_copy(curthread->t_vmspace, &new_addr);
	if (result) {
		kfree(new_tf);
		new_tf = NULL;
		return ENOMEM;
	}

	/* Call thread_fork(	) */
	result = thread_fork(curthread->t_name, new_tf, 0, new_addr, new_thread);

	if (result != 0) { /* failed in thread_fork */
	    kfree(new_tf);
	    new_tf = NULL;
	    kfree(new_addr);
	    new_addr = NULL;

	    return result;
	}

	*retval = new_thread->t_pid;

	return 0;
}


int waitpid(pid_t pid, int *returncode, int flags) {


	return 0;
}
