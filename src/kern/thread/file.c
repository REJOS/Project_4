#include <file.h>
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <vm.h>
#include <vfs.h>
#include <vnode.h>
#include <stdarg.h>
#include <uio.h>
#include <synch.h>
#include <kern/stat.h>

int file_open(char *filename, int flags, int mode, int *retfd) {



	return 0;
}


int file_close(int fd) {



	return 0;
}


int filetable_init(const char *inpath, const char *outpath, const char *errpath) {

	curthread->t_filetable = kmalloc(sizeof(struct filetable));

	if (curthread->t_filetable == NULL) {
		return ENOMEM;
	}

	int i;
	for (i = 0; i < OPEN_MAX; i++) {
		curthread->t_filetable->ft_openfiles[i] = NULL;
	}

	return 0;
}


int filetable_copy(struct filetable **copy) {



	return 0;
}


int filetable_placefile(struct openfile *file, int *fd) {

	int i;
	for (i = 0; i < OPEN_MAX; i++) {
		if (curthread->t_filetable->ft_openfiles[i] == NULL) {
			curthread->t_filetable->ft_openfiles[i] = file;
			*fd = i;
			return 0;
		}
	}

	return -1;
}


int filetable_findfile(int fd, struct openfile **file) {

	if (fd >= OPEN_MAX || fd < 0) {
		file = NULL;
		return -1;
	}

	*file = curthread->t_filetable->ft_openfiles[fd];

	return 0;
}


int filetable_dup2file(int oldfd, int newfd) {



	return 0;
}


void filetable_destroy(struct filetable *ft) {



}


