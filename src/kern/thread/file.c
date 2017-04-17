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

int file_open(char *filename, int flags, int *retfd) {
	int result = 0;
	struct vnode *vn;
	char *kbuf;
	size_t len;
	kbuf = (char *) kmalloc(sizeof(char)*PATH_MAX);
	result = copyinstr((const_userptr_t)filename,kbuf,PATH_MAX,&len);
	if(result){
		kfree(kbuf);
		return result;
	}
	struct openfile **file;
	(*file)->of_vnode = vn;
	(*file)->of_refcount = 1;
	(*file)->of_offset = 0;
	(*file)->of_lock = lock_create(kbuf);

	result = vfs_open(kbuf,flags,&vn);
	if(result) {
		kfree(kbuf);
		kfree(*file);
		return result;
	}

	kfree(kbuf);
	return filetable_placefile(*file, retfd);

}


int file_close(int fd) {

	struct openfile **file;
	filetable_findfile(fd, file);

	if(fd >= OPEN_MAX || fd < 0){
		return EBADF;
	}

	if(*file == NULL){
		return EBADF;
	}

	if((*file)->of_vnode == NULL){
		return EBADF;
	}

	if((*file)->of_refcount == 1){
		VOP_CLOSE((*file)->of_vnode);
		lock_destroy((*file)->of_lock);
		kfree(*file);
	}else{
		(*file)->of_refcount -= 1;
	}
	return 0;
}


int filetable_init(struct filetable *ft) {

	ft = kmalloc(sizeof(struct filetable));

	if (ft == NULL) {
		return ENOMEM;
	}

	int i;
	for (i = 0; i < OPEN_MAX; i++) {
		ft->ft_openfiles[i] = NULL;
	}

	return 0;
}


int filetable_copy(struct filetable **copy) {

	int result;

	result = filetable_init(*copy);

	if (result) {
		return result;
	}

	int i;
	for (i = 0; i < OPEN_MAX; i++) {
		if (curthread->t_filetable->ft_openfiles[i] != NULL) {
			(*copy)->ft_openfiles[i] = kmalloc(sizeof(struct openfile));
			if ((*copy)->ft_openfiles[i] == NULL) {
				return ENOMEM;
			}
			(*copy)->ft_openfiles[i]->of_vnode = curthread->t_filetable->ft_openfiles[i]->of_vnode;
			(*copy)->ft_openfiles[i]->of_lock = curthread->t_filetable->ft_openfiles[i]->of_lock;
			(*copy)->ft_openfiles[i]->of_offset = curthread->t_filetable->ft_openfiles[i]->of_offset;
			(*copy)->ft_openfiles[i]->of_refcount = curthread->t_filetable->ft_openfiles[i]->of_refcount;
		}
	}

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
	int result = 0;
	if(oldfd >= OPEN_MAX || oldfd < 0 || newfd >= OPEN_MAX || newfd < 0){
		return EBADF;
	}
	if(oldfd == newfd){
		return 0;
	}
	struct openfile **fileO;
	filetable_findile(oldfd,fileO);
	if(*fileO == NULL){
		return EBADF;
	}
	struct openfile **fileN;
	filetable_findile(oldfd,fileN);
	if(fileN != NULL){
		result = filetable_close(newfd);
		if(result){
			return EBADF;
		}
	}
	lock_acquire((*fileO)->of_lock);
	(*fileN)->of_vnode = (*fileO)->of_vnode;
	(*fileN)->of_offset = (*fileO)->of_offset;
	(*fileN)->of_accmode = (*fileO)->of_accmode;
	(*fileN)->of_refcount = (*fileO)->of_refcount;
	(*fileN)->of_lock = lock_create("DUP2");
	lock_release((*fileO)->of_lock);
	return 0;
}


void filetable_destroy(struct filetable *ft) {

	int i;
	for (i = 0; i < OPEN_MAX; i++) {
		if (ft->ft_openfiles[i] != NULL) {
			kfree(ft->ft_openfiles[i]);
		}
	}

}


