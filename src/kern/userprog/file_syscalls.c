#include <file.h>
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

typedef int mode_t;

int sys_open(const char *path, int oflag, mode_t mode) {



	return 0;
}


int sys_read(int fd, void *buf, size_t nbytes) {
	int result=0;

	if(filehandle >= OPEN_MAX || filehandle < 0 || curthread->fdtable[filehandle] == NULL || curthread->fdtable[filehandle]->flags == O_WRONLY) {
		return EBADF;
	}

	struct iovec iov;
	struct uio ku;
	void *kbuf;
	kbuf = kmalloc(sizeof(*buf)*size);
	if(kbuf == NULL) {
		return EINVAL;
	}

	lock_acquire(curthread->fdtable[filehandle]->lk);

	iov.iov_ubase = (userptr_t)buf;
	iov.iov_len = size;
	ku.uio_iov = &iov;
	ku.uio_iovcnt = 1;
	ku.uio_offset = curthread->fdtable[filehandle]->offset;
	ku.uio_resid = size;
	ku.uio_segflg = UIO_USERSPACE;
	ku.uio_rw = UIO_READ;
	ku.uio_space = curthread->t_addrspace;


	result = VOP_READ(curthread->fdtable[filehandle]->vn, &ku);
	if(result) {
		kfree(kbuf);
		lock_release(curthread->fdtable[filehandle]->lk);
		return result;
	}

	curthread->fdtable[filehandle]->offset = ku.uio_offset;

	kfree(kbuf);
	lock_release(curthread->fdtable[filehandle]->lk);
	return 0;
}


int sys_write(int fd, const void *buf, size_t nbytes) {
	int result=0;

	if(filehandle >= OPEN_MAX || filehandle < 0 || curthread->fdtable[filehandle] == NULL || curthread->fdtable[filehandle]->flags == O_RDONLY) {
		return EBADF;
	}

	struct iovec iov;
	struct uio ku;
	void *kbuf;
	kbuf = kmalloc(sizeof(*buf)*size);
	if(kbuf == NULL) {
		return EINVAL;
	}

	lock_acquire(curthread->fdtable[filehandle]->lk);

	result = copyin((const_userptr_t)buf,kbuf,size);
	if(result) {
		kfree(kbuf);
		lock_release(curthread->fdtable[filehandle]->lk);
		return result;
	}

	iov.iov_ubase = (userptr_t)buf;
	iov.iov_len = size;
	ku.uio_iov = &iov;
	ku.uio_iovcnt = 1;
	ku.uio_offset = curthread->fdtable[filehandle]->offset;
	ku.uio_resid = size;
	ku.uio_segflg = UIO_USERSPACE;
	ku.uio_rw = UIO_WRITE;
	ku.uio_space = curthread->t_addrspace;

	result = VOP_WRITE(curthread->fdtable[filehandle]->vn, &ku);
	if(result) {
		kfree(kbuf);
		lock_release(curthread->fdtable[filehandle]->lk);
		return result;
	}

	curthread->fdtable[filehandle]->offset = ku.uio_offset;

	kfree(kbuf);
	lock_release(curthread->fdtable[filehandle]->lk);
	return 0;

}


int sys_lseek(int fd, off_t offset, int whence) {



	return 0;
}


int sys_close(int fd) {



	return 0;
}


int sys_dup2file(int oldfd, int newfd) {



	return 0;
}
