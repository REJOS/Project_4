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
#include <vm.h>
#include <vfs.h>
#include <vnode.h>
#include <stdarg.h>
#include <uio.h>
#include <synch.h>
#include <kern/stat.h>



int sys_open(const char *path, int oflag) {
	int * retfd;
	file_open(path,oflag,retfd);
	return *retfd;
}

int sys_read(int fd, void *buf, size_t nbytes) {

	int result=0;
	if(fd >= OPEN_MAX || fd < 0){
		return EBADF;
	}

	struct openfile **file;
	filetable_findfile(fd, file);
	if(*file == NULL){
		return EBADF;
	}

	if((*file)->of_accmode == O_WRONLY){
		return EBADF;
	}

	void *kbuf;
	kbuf = kmalloc(sizeof(*buf)*nbytes);
	if(kbuf == NULL){
		return EINVAL;
	}

	struct iovec v;
	struct uio u;

	lock_acquire((*file)->of_lock);
	v.iov_ubase = (userptr_t)buf;
	v.iov_len = nbytes;
	u.uio_iovec = v;
	u.uio_offset = (*file)->of_offset;
	u.uio_resid = nbytes;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = curthread->t_vmspace;

	result = VOP_READ((*file)->of_vnode,&u);
	if(result){
		kfree(kbuf);
		lock_release((*file)->of_lock);
		return result;
	}
	(*file)->of_offset = u.uio_offset;
	result = nbytes - u.uio_resid;
	kfree(kbuf);
	lock_release((*file)->of_lock);
	return result;
}


int sys_write(int fd, const void *buf, size_t nbytes) {
	int result = 0;
	if(fd >= OPEN_MAX || fd < 0){
		return EBADF;
	}
	struct openfile **file;
	filetable_findfile(fd, file);
	if(*file == NULL){
		return EBADF;
	}

	if((*file)->of_accmode == O_RDONLY){
		return EBADF;
	}

	void *kbuf;
		kbuf = kmalloc(sizeof(*buf)*nbytes);
		if(kbuf == NULL){
			return EINVAL;
		}

		struct iovec v;
		struct uio u;

		lock_acquire((*file)->of_lock);
		v.iov_ubase = (userptr_t)buf;
		v.iov_len = nbytes;
		u.uio_iovec = v;
		u.uio_offset = (*file)->of_offset;
		u.uio_resid = nbytes;
		u.uio_segflg = UIO_USERSPACE;
		u.uio_rw = UIO_READ;
		u.uio_space = curthread->t_vmspace;

		result = VOP_WRITE((*file)->of_vnode,&u);
		if(result){
			kfree(kbuf);
			lock_release((*file)->of_lock);
			return result;
		}
		(*file)->of_offset = u.uio_offset;
		result = nbytes - u.uio_resid;
		kfree(kbuf);
		lock_release((*file)->of_lock);
		return result;

	return 0;

}


int sys_lseek(int fd, off_t offset, int whence) {
	int result = 0;

	struct openfile **file;
	filetable_findfile(fd, file);

	if(fd >= OPEN_MAX || fd < 0){
		return EBADF;
	}

	if(*file == NULL){
		return EBADF;
	}

	off_t pos, file_size;
	struct stat statbuf;

	lock_acquire((*file)->of_lock);
	result = VOP_STAT((*file)->of_vnode, &statbuf);
	if(result){
		lock_release((*file)->of_lock);
		return result;
	}
	file_size = statbuf.st_size;

	if(whence == SEEK_SET){
		result = VOP_TRYSEEK((*file)->of_vnode,offset);
		if(result){
			lock_release((*file)->of_lock);
			return result;
		}
		pos = offset;
	}

	else if(whence == SEEK_CUR){
		result = VOP_TRYSEEK((*file)->of_vnode, (*file)->of_offset + offset);
		if(result){
			lock_release((*file)->of_lock);
			return result;
		}
		pos = (*file)->of_offset+offset;
	}

	else if(whence == SEEK_END){
		result = VOP_TRYSEEK((*file)->of_vnode,file_size+offset);
		if(result){
			lock_release((*file)->of_lock);
			return result;
		}
		pos = file_size + offset;
	}
	else{
		lock_release((*file)->of_lock);
		return EINVAL;
	}

	if(pos < (off_t)0){
		lock_release((*file)->of_lock);
		return EINVAL;
	}
	(*file)->of_offset = pos;

	lock_release((*file)->of_lock);


	return 0;
}


int sys_close(int fd) {
	return file_close(fd);
}


int sys_dup2file(int oldfd, int newfd) {
	return filetable_dup2file(oldfd, newfd);
}
