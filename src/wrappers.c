#include "wrappers.h"

void *xmalloc (size_t size) {
	void *value = malloc (size);
	if (value == 0) {
		perror("malloc"); exit(errno);
	}
	return value;
}

void *xcalloc (size_t nmemb, size_t size) {
	void *value = calloc (nmemb, size);
	if (value == 0) {
		perror("cmalloc"); exit(errno);
	}
	return value;
}
void *xrealloc(void *ptr, size_t size) {
	void *value = realloc (ptr, size);
	if (value == 0) {
		perror("realloc"); exit(errno);
	}
	return value;
}

pid_t xfork(void) {
	pid_t value;
	if ((value = fork()) == -1) {
		perror("fork"); exit(errno);
	}
	return value;
}

int xclose(int fd) {
	if (close(fd) == -1) {
		perror("close"); exit(errno);
	}
	return 0;
}

int xdup2(int oldfd, int newfd) {
	int rtn;
	if ((rtn = dup2(oldfd, newfd)) == -1) {
		perror("dup2"); exit(errno);
	}
	return rtn;
}

void loopArray(void **arr, void (*fn)(void *)) {
	unsigned int i;
	if (arr != NULL) {
		for (i = 0; arr[i] != NULL; i++) {
			fn(arr[i]); /* Execute function on element */
		}
	} 
}

char *xgetcwd() {
	char *buf = NULL;
	size_t size;
	buf = getcwd(buf,0);
	if (!buf) {
		perror("getcwd");
	}
	return buf;
}
/*loopArray((void**)args, (void*)&printString);*/
void printString(char * str) {
	printf("%s\n", str);
}
