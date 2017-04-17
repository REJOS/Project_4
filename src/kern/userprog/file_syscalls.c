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
#include <vnode.h>
#include <stdarg.h>
#include <types.h>
#include <uio.h>
#include <synch.h>
#include <kern/stat.h>

typedef int mode_t;

int sys_open(const char *path, int oflag, mode_t mode) {
	return 0;
}


int sys_read(int fd, void *buf, size_t nbytes) {

	return 0;
}


int sys_write(int fd, const void *buf, size_t nbytes) {

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
