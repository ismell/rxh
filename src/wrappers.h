#ifndef wrappers_H
#define wrappers_H
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

void *xmalloc (size_t size);
void *xcalloc (size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
pid_t xfork(void);
int xclose(int fd);
int xdup2(int oldfd, int newfd);
void loopArray(void **arr, void (*fn)(void *));
char *xgetcwd();
void printString(char *);

#endif
