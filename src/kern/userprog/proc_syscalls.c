#include <types.h>
#include <lib.h>
#include <syscall.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <kern/errno.h>
#include <machine/trapframe.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <kern/errno.h>
#include <vm.h>
#include <vfs.h>
//#include <file.h>
#include <test.h>
#include <synch.h>

static struct argvdata argdata;


void sys__exit(int code) {

	pid_setexitstatus(curthread->t_pid, code);

	thread_exit();

}


/*
 * sys_execv
 * 1. copyin the program name
 * 2. copyin_args the argv
 * 3. load the executable
 * 4. copyout_args the argv
 * 5. warp to usermode
 */
int
sys_execv(userptr_t prog, userptr_t argv)
{

	char *path;
	vaddr_t entrypoint, stackptr;
	int argc;
	int result;

	path = kmalloc(PATH_MAX);
	if (path == NULL) {
		return ENOMEM;
	}

	/* get the filename. */
	result = copyinstr(prog, path, PATH_MAX, NULL);
	if (result) {
		kfree(path);
		return result;
	}

	/* get the argv strings. */

	lock_acquire(argdata.lock);

	/* allocate the space */
	argdata.buffer = kmalloc(ARG_MAX);
	if (argdata.buffer == NULL) {
		lock_release(argdata.lock);
		kfree(path);
		return ENOMEM;
	}
	argdata.offsets = kmalloc(NARG_MAX * sizeof(size_t));
	if (argdata.offsets == NULL) {
		kfree(argdata.buffer);
		lock_release(argdata.lock);
		kfree(path);
		return ENOMEM;
	}

	/* do the copyin */
	result = copyin_args(argv, argdata);
	if (result) {
		kfree(path);
		kfree(argdata.buffer);
		kfree(argdata.offsets);
		lock_release(argdata.lock);
		return result;
	}

	/* load the executable. Note: must not fail after this succeeds. */
	result = loadexec(prog, entrypoint, stackptr);
	if (result) {
		kfree(path);
		kfree(argdata.buffer);
		kfree(argdata.offsets);
		lock_release(argdata.lock);
		return result;
	}

	/* don't need this any more */
	kfree(path);

	/* send the argv strings to the process. */
	result = copyout_args(argdata, argv, stackptr);
	if (result) {
		lock_release(argdata.lock);

		/* if copyout fails, *we* messed up, so panic */
		panic("execv: copyout_args failed: %s\n", strerror(result));
	}
	argc = argdata.nargs;

	/* free the argdata space */
	kfree(argdata.buffer);
	kfree(argdata.offsets);

	lock_release(argdata.lock);

	/* warp to user mode. */
	md_usermode(argc, argv, stackptr, entrypoint);

	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
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
	result = thread_fork(curthread->t_name, new_tf, 0, (unsigned long)new_addr, &new_thread);

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


int sys_waitpid(pid_t pid, int *returncode, int flags) {

	pid_t *retval;

	*returncode = pid_wait(pid, returncode, flags, retval);

	return 0;
}

void
execv_bootstrap(void)
{
	argdata.lock = lock_create("arglock");
	if (argdata.lock == NULL) {
		panic("Cannot create argv data lock\n");
	}
}

void
execv_shutdown(void)
{
	lock_destroy(argdata.lock);
}

/*
 * copyin_args
 * copies the argv into the kernel space argvdata.
 * read through the comments to see how it works.
 */
int
copyin_args(userptr_t argv, struct argvdata *ad)
{
	userptr_t argptr;
	size_t arglen;
	size_t bufsize, bufresid;
	int result;

	assert(lock_do_i_hold(ad->lock));

	/* for convenience */
	bufsize = bufresid = ARG_MAX;

	/* reset the argvdata */
	ad->bufend = ad->buffer;

	/* loop through the argv, grabbing each arg string */
	for (ad->nargs = 0; ad->nargs <= NARG_MAX; ad->nargs++) {

		/*
		 * first, copyin the pointer at argv
		 * (shifted at the end of the loop)
		 */
		result = copyin(argv, &argptr, sizeof(userptr_t));
		if (result) {
			return result;
		}

		/* if the argptr is NULL, we hit the end of the argv */
		if (argptr == NULL) {
			break;
		}

		/* too many args? bail */
		if (ad->nargs >= NARG_MAX) {
			return E2BIG;
		}

		/* otherwise, copyinstr the arg into the argvdata buffer */
		result = copyinstr(argptr, ad->bufend, bufresid, &arglen);
		if (result == ENAMETOOLONG) {
			return E2BIG;
		}
		else if (result) {
			return result;
		}

		/* got one -- update the argvdata and the local argv userptr */
		ad->offsets[ad->nargs] = bufsize - bufresid;
		ad->bufend += arglen;
		bufresid -= arglen;
		argv += sizeof(userptr_t);
	}

	return 0;
}

/*
 * copyout_args
 * copies the argv out of the kernel space argvdata into the userspace.
 * read through the comments to see how it works.
 */
int
copyout_args(struct argvdata *ad, userptr_t *argv, vaddr_t *stackptr)
{
	userptr_t argbase, userargv, arg;
	vaddr_t stack;
	size_t buflen;
	int i, result;

	assert(lock_do_i_hold(ad->lock));

	/* we use the buflen a lot, precalc it */
	buflen = ad->bufend - ad->buffer;

	/* begin the stack at the passed in top */
	stack = *stackptr;

	/*
	 * copy the block of strings to the top of the user stack.
	 * we can do it as one big blob.
	 */

	/* figure out where the strings start */
	stack -= buflen;

	/* align to sizeof(void *) boundary, this is the argbase */
	stack -= (stack & (sizeof(void *) - 1));
	argbase = (userptr_t)stack;

	/* now just copyout the whole block of arg strings  */
	result = copyout(ad->buffer, argbase, buflen);
	if (result) {
		return result;
	}

	/*
	 * now copy out the argv itself.
	 * the stack pointer is already suitably aligned.
	 * allow an extra slot for the NULL that terminates the vector.
	 */
	stack -= (ad->nargs + 1)*sizeof(userptr_t);
	userargv = (userptr_t)stack;

	for (i = 0; i < ad->nargs; i++) {
		arg = argbase + ad->offsets[i];
		result = copyout(&arg, userargv, sizeof(userptr_t));
		if (result) {
			return result;
		}
		userargv += sizeof(userptr_t);
	}

	/* NULL terminate it */
	arg = NULL;
	result = copyout(&arg, userargv, sizeof(userptr_t));
	if (result) {
		return result;
	}

	*argv = (userptr_t)stack;
	*stackptr = stack;
	return 0;
}

/*
 * loadexec
 * common code for execv and runprogram: loading the executable
 */
int
loadexec(char *path, vaddr_t *entrypoint, vaddr_t *stackptr)
{
	struct addrspace *newvm, *oldvm;
	struct vnode *v;
	char *newname;
	int result;

	/* new name for thread */
	newname = kstrdup(path);
	if (newname == NULL) {
		return ENOMEM;
	}

	/* open the file. */
	result = vfs_open(path, O_RDONLY, &v);
	if (result) {
		kfree(newname);
		return result;
	}

	/* make a new address space. */
	newvm = as_create();
	if (newvm == NULL) {
		vfs_close(v);
		kfree(newname);
		return ENOMEM;
	}

	/* replace address spaces, and activate the new one */
	oldvm = curthread->t_vmspace; /* backup t_vmspace of curthread */
	curthread->t_vmspace = newvm; /* setup t_vmspace of curthread to newvm */
	as_activate(curthread->t_vmspace); /* activate t_vmspace of curthread */

	/* load the executable. if it fails, restore the old address space and
	 * activate it.
	 */
	result = load_elf(&v, entrypoint);
	if (result) {
		vfs_close(v);
		curthread->t_vmspace = oldvm;
		as_activate(curthread->t_vmspace);
		as_destroy(newvm);
		kfree(newname);
		return result;
	}

	/* done with the file */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, stackptr);
	if (result) {
		curthread->t_vmspace = oldvm;
		as_activate(curthread->t_vmspace);
		as_destroy(newvm);
		kfree(newname);
		return result;
	}

	/*
	 * wipe out old address space
	 *
	 * note: once this is done, execv() must not fail, because there's
	 * nothing left for it to return an error to.
	 */
	if (oldvm) {
		as_destroy(oldvm); /* wipe out old address space oldvm */
	}

	/*
	 * Now that we know we're succeeding, change the current thread's
	 * name to reflect the new process.
	 */
	kfree(curthread->t_name);
	curthread->t_name = newname; /* set t_name to newname for thread */

	return 0;
}

void init_argdata(void){
	struct argvdata *ptr = kmalloc(sizeof(struct argvdata));

	argdata = *ptr;
	argdata.buffer = NULL;
	argdata.bufend = NULL;
	argdata.offsets = NULL;
	argdata.nargs = 0;
	argdata.lock = lock_create("argdata lock");
}
