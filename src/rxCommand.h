#ifndef rxCommand_H
#define rxCommand_H
#define DEBUG 0

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/* rxType: is the type of command to execute
		   rxExec: Execute a command
		   rxFunction: Execute an internal function
*/
//typedef enum {rxExec, rxFunction} rxType;

typedef struct {
	int id; /* This is used for debuging purposes only */
	pid_t pid;
	char** args;
	int fd[2];
	int status; /* Done: 0, Running: !0 */
	int (*fn)(char **);
	char *op;
} rxCommand;

void rxCmd_Print(rxCommand const *cmd);
void rxCmd_Free(rxCommand *cmd);
int rxCmd_ToString(rxCommand **cmds, char **buf);
int rxCmd_Sub(char *input, char **cmd, int *i);
int rxCmd_Execute(rxCommand **cmds);
rxCommand **rxParse(char *input);
char *rxExpandVars(char *input);


#endif
