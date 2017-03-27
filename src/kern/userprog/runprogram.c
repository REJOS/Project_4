/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>
#include <syscall.h>


static struct argvdata argdata;
/*
 * runprogram
 * load program "progname" and start running it in usermode.
 * does not return except on error.
 *
 * opens the std file descriptors if necessary.
 *
 * note -- the pathname must be mutable, since it passes it to loadexec which
 * passes it to vfs_open.
 */
int
runprogram(char *progname)
{
	//init_argdata();
	vaddr_t entrypoint, stackptr;
	int argc;
	userptr_t argv;
	int result;

	/* we must be a user process thread */
	assert(curthread->t_pid >= PID_MIN && curthread->t_pid <= PID_MAX);

	/* we should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* set up stdin/stdout/stderr. */
	/*if (curthread->t_filetable == NULL) {
		result = filetable_init("con:", "con:", "con:");
		if (result) {
			return result;
		}
	}*/

	lock_acquire(argdata.lock);

	/* make up argv strings */

	if (strlen(progname) + 1 > ARG_MAX) {
		lock_release(argdata.lock);
		return E2BIG;
	}

	/* allocate the space */
	argdata.buffer = kmalloc(strlen(progname) + 1);
	if (argdata.buffer == NULL) {
		lock_release(argdata.lock);
		return ENOMEM;
	}
	argdata.offsets = kmalloc(sizeof(size_t));
	if (argdata.offsets == NULL) {
		kfree(argdata.buffer);
		lock_release(argdata.lock);
		return ENOMEM;
	}

	/* copy it in, set the single offset */
	strcpy(argdata.buffer, progname);
	argdata.bufend = argdata.buffer + (strlen(argdata.buffer) + 1);
	argdata.offsets[0] = 0;
	argdata.nargs = 1;

	/* load the executable. note: must not fail after this succeeds. */
	result = loadexec(progname, &entrypoint, &stackptr);
	if (result) {
		kfree(argdata.buffer);
		kfree(argdata.offsets);
		lock_release(argdata.lock);
		return result;
	}

	result = copyout_args(&argdata, &argv, &stackptr);
	if (result) {
		kfree(argdata.buffer);
		kfree(argdata.offsets);
		lock_release(argdata.lock);

		/* If copyout fails, *we* messed up, so panic */
		panic("execv: copyout_args failed: %s\n", strerror(result));
	}

	argc = argdata.nargs;

	/* free the space */
	kfree(argdata.buffer);
	kfree(argdata.offsets);

	lock_release(argdata.lock);

	/* warp to user mode. */
	md_usermode(argc, argv, stackptr, entrypoint);

	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL; /* quell compiler warning */
}

