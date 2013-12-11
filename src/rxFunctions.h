#ifndef rxFunctions_H
#define rxFunctions_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "rxJobs.h"
#include <dirent.h>
#include <sys/dir.h>
#include <sys/ioctl.h>

int rxf_cd (char **);
int rxf_ls (char **);
int rxf_enviorn (char **);
int rxf_echo (char **);
int rxf_export (char **);
int rxf_jobs (char **);
int rxf_exit (char **);

#endif
