#ifndef rxJobs_H
#define rxJobs_H

#ifndef DEBUG
	#define DEBUG 0
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "wrappers.h"


typedef enum {rxRunning, rxDone} rxStatus;

typedef struct {
	int id; /* This is assigned by the add command */
	pid_t pid;
	int status; /* 0: done, 1: Running */
	int rtn;
	char *cmd;
} rxJob;

typedef struct {
	int i;
	int n;
	rxJob **jobs;
	int cnt;
} rxJobs;

rxJobs *rxJobs_Initialize();
void rxJob_Print(rxJob *job);
int rxJob_Add(rxJobs *jobs, rxJob *job);
void rxJobs_Wait(rxJobs *jobs, int hang);
#endif
