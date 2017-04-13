
int file_open(char *filename, int flags, int mode, int *retfd) {



	return 0;
}


int file_close(int fd) {



	return 0;
}


int filetable_init(const char *inpath, const char *outpath, const char *errpath) {



	return 0;
}


int filetable_copy(struct filetable **copy) {



	return 0;
}


int filetable_placefile(struct openfile *file, int *fd) {



	return 0;
}


int filetable_findfile(int fd, struct openfile **file) {



	return 0;
}


int filetable_dup2file(int oldfd, int newfd) {



	return 0;
}


void filetable_destroy(struct filetable *ft) {



}


