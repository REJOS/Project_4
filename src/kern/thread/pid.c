#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <array.h>
#include <syscall.h>
#include <thread.h>
#include <curthread.h>
#include <synch.h>

static pid_t nextpid;

static struct lock * pidlock;

static struct array * pidinfoTable;

int findpidindex(pid_t pid, int *retval);


int pid_initialize(void) {

	nextpid = 0;

	pidlock = lock_create("pid lock");

	pidinfoTable = array_create();
	if (pidinfoTable==NULL) {
		panic("Cannot create pidinfoTable array\n");
	}

	return 0;
}

int pid_create(pid_t *retval, pid_t ppid, int status) {

	assert(ppid >= 0);

	lock_acquire(pidlock);

	struct pidinfo *new_info = kmalloc(sizeof(struct pidinfo));
	if (new_info == NULL) {
		kfree(new_info);
		new_info = NULL;
		return -1;
	}

	new_info->pid = nextpid;

	new_info->parent_pid = ppid;

	new_info->exitstatus = status;

	new_info->exitcv = cv_create("exitstatus cv");

	int result = array_add(pidinfoTable, new_info);

	if (result) {
		kfree(new_info);
		new_info = NULL;
		return -2;
	}

	*retval = nextpid;

	nextpid++;

	lock_release(pidlock);

	return 0;
}

int pid_destroy(pid_t dpid) {

	lock_acquire(pidlock);

	int *i;

	int result = findpidindex(dpid, i);

	if (result) {
		return result;
	}

	struct pidinfo *pidinfotodestroy = array_getguy(pidinfoTable, *i);

	cv_destroy(pidinfotodestroy->exitcv);

	kfree(pidinfotodestroy);
	pidinfotodestroy = NULL;

	array_remove(pidinfoTable, *i);

	lock_release(pidlock);

	return 0;
}

int pid_get(pid_t gpid, pid_t *ppid, int *status) {

	lock_acquire(pidlock);

	int *i;

	int result = findpidindex(gpid, i);

	if (result) {
		return result;
	}

	struct pidinfo *cur_pidinfo = array_getguy(pidinfoTable, *i);

	*ppid = cur_pidinfo->parent_pid;

	*status = cur_pidinfo->exitstatus;

	lock_release(pidlock);

	return 0;
}

int pid_wait(pid_t wpid, int *status, int flags, pid_t *ret) {

	if (wpid == curthread->t_pid) {
		return EINVAL;
	}

	if(flags) {
		return EINVAL;
	}

	if(status == NULL) {
		return EFAULT;
	}

	lock_acquire(pidlock);

	int *i;

	int result = findpidindex(wpid, i);

	if (result) {
		return result;
	}

	struct pidinfo *cur_pidinfo = array_getguy(pidinfoTable, *i);

	if (cur_pidinfo->parent_pid == curthread->t_pid) {
		lock_release(pidlock);
		return EINVAL;
	}

	if (cur_pidinfo->exitstatus != 0) {
		cv_wait(cur_pidinfo->exitcv, pidlock);
	}

	*status = cur_pidinfo->exitstatus;
	*ret = wpid;

	lock_release(pidlock);

	return 0;
}

int pid_setexitstatus(pid_t spid, int status) {

	lock_acquire(pidlock);

	int *i;

	int result = findpidindex(spid, i);

	if (result) {
		return result;
	}

	struct pidinfo *cur_pidinfo = array_getguy(pidinfoTable, *i);

	cur_pidinfo->exitstatus = status;

	if (status)	{
		cv_broadcast(cur_pidinfo->exitcv, pidlock);
	}

	lock_release(pidlock);

	return 0;
}

//private method
int findpidindex(pid_t pid, int *retval) {

	int i;

	for (i=0; i<array_getnum(pidinfoTable); i++) {
		struct thread *p = array_getguy(pidinfoTable, i);
		if (p->t_pid == pid) {
			*retval = i;
			return 0;
		}
	}

	//not found
	return -1;
}

