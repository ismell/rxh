#include "rxCommand.h"

void rxCmd_Print(rxCommand const *cmd) {
	unsigned int i;
	if (cmd != NULL) {
		printf("id: %d\n", cmd->id);
		printf("pid: %d\n", cmd->pid);
		if (cmd->args != NULL) {
			for (i = 0; cmd->args[i] != NULL; i++) {
				printf("arg%d: %s\n",i, cmd->args[i]);
			}
		}
		printf("fd0: %d\nfd1: %d\n", cmd->fd[0], cmd->fd[1]);
		printf("Op: %s\n\n", cmd->op);
		//printf("Type: %d\n\n", cmd->type);
	}
}

void rxCmd_Free(rxCommand *cmd) {
	unsigned int i;
	if (cmd != NULL) {
		if (cmd->args != NULL) {
			for (i = 0; cmd->args[i] != NULL; i++) {
				free(cmd->args[i]); /* Free individual arguments */
			}
			free(cmd->args); /* Free args array */
		}
		if (cmd->op != NULL) {
			free(cmd->op);
		}
		free(cmd);
	}	
}

int rxCmd_Execute(rxCommand **cmds) {
	if (cmds == NULL)
		return -1;

	rxCommand *curCmd, *prevCmd, *nextCmd;
	int cpid, status = 0;
	unsigned int i;

	for (i = 0; cmds[i] != NULL; i++) {
		curCmd = cmds[i];
		prevCmd = (i > 0 ? cmds[i-1] : NULL);
		nextCmd = cmds[i+1]; // We can do this cuz we have a NULL terminated array

		curCmd->status = 1;

		/* we will be redirecting something */
		if (nextCmd != NULL) { 
			if (!strcmp(curCmd->op, "|") || (!strcmp(curCmd->op, "<") && !strcmp(nextCmd->op, "|"))) { /* Only create a pipe if we have something else to pipe into */
				if (pipe(curCmd->fd) == -1) { /* Create pipe*/
					perror("pipe"); exit(EXIT_FAILURE);
				}
				if (DEBUG) fprintf(stderr, "%d-Pipe Created\n", curCmd->id);
			}
		}

		/* Fork process */
		if (!nextCmd && !prevCmd && curCmd->fn) {
			if (DEBUG) fprintf(stderr, "Executing internal function on main thread\n");
			curCmd->pid = -1;
		}
		else if ((curCmd->pid = fork()) == -1) {
			perror("fork"); exit(EXIT_FAILURE);
		}

		if (curCmd->pid == 0) { /* We are the child */
			if (prevCmd != NULL && !strcmp(prevCmd->op, "|")) { /* We are getting something piped into us */
				if (DEBUG) fprintf(stderr, "%d-Somethings piping into us\n", curCmd->id);
				/* Close output side of the previous pipe */
				xclose(prevCmd->fd[1]);

				/* Replace stdin with input side of the previous pipe */
				xdup2(prevCmd->fd[0], 0);
			}
			else if (nextCmd != NULL && !strcmp(curCmd->op, "<")) { /* We are getting a file piped into us */
				if (DEBUG) fprintf(stderr, "%d-Something redirecting into us\n", curCmd->id);
				if ((curCmd->fd[0] = open(nextCmd->args[0], O_RDONLY)) == -1) {
					perror("open"); exit(errno);
				}

				/* Replace stdin with file descriptor */
				xdup2(curCmd->fd[0], 0);
				
				/* Update nextCmd now that this is used */
				free(curCmd->op);
				curCmd->op = strdup(nextCmd->op); // Change current command operator to nextCmd operator
				if (DEBUG) fprintf(stderr, "%d-New Op: %s\n", curCmd->id, curCmd->op);
				nextCmd = cmds[i+2];	//We know the next command is not null, so we can increment 2 and be safe
			}

			if (nextCmd != NULL) {
				if (!strcmp(curCmd->op, "|")) { /* We are piping to another process */
					if (DEBUG) fprintf(stderr, "%d-We are piping to another process\n", curCmd->id);
					/* Close input side of the current pipe */
					xclose(curCmd->fd[0]);

					/* Replace stdout with output side of the pipe */
					xdup2(curCmd->fd[1], 1);
				}
				else if (!strcmp(curCmd->op, ">") || !strcmp(curCmd->op, ">>")) { /* We are piping to a file */
					if (DEBUG) fprintf(stderr, "%d-We are redirecting to a file\n", curCmd->id);
					if ((curCmd->fd[1] = open(nextCmd->args[0], O_CREAT|O_WRONLY|(!strcmp(curCmd->op, ">>") ? O_APPEND : O_TRUNC), S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) == -1) {
						perror("open"); exit(errno);
					}

					/* Replace stdout with file descriptor */
					xdup2(curCmd->fd[1], 1);
				}
			}
			if (curCmd->fn) {
				if (DEBUG) fprintf(stderr, "%d-Executing Forked Internal Command %s\n", curCmd->id, curCmd->args[0]);
				exit(curCmd->fn(curCmd->args));
			} else {
				if (DEBUG) fprintf(stderr, "%d-Executing %s\n", curCmd->id, curCmd->args[0]);
				execvp(curCmd->args[0], curCmd->args);
				perror("exec"); exit(errno);
			}

		}
		else if (curCmd->pid == -1) { /* Single internal command */
			if (DEBUG) fprintf(stderr, "Executing internal function\n");
			curCmd->status = 0;
			return curCmd->fn(curCmd->args);
		} else { // We are the parent
			if (DEBUG) fprintf(stderr, "0-Child pid: %d\n", curCmd->pid);
			if (prevCmd != NULL && !strcmp(prevCmd->op, "|")) { /* Close the previous pipes, the parent no longer needs them */
				if (DEBUG) fprintf(stderr, "0-Closing %d pipes\n", curCmd->pid);
				/* Close output side of the previous pipe */
				xclose(prevCmd->fd[1]);
				/* Close input side of the previous pipe */
				xclose(prevCmd->fd[0]);
			}
			if (!strcmp(curCmd->op, "<")) { /* the child redirected to a file */
				nextCmd->fd[0] = curCmd->fd[0];
				nextCmd->fd[1] = curCmd->fd[1];
				i++; // we skip over the next element because its been used
			}
			if (cmds[i] != NULL && (!strcmp(cmds[i]->op, ">>") || !strcmp(cmds[i]->op, ">")))
				i++; // we skip over this element so we dont process the redirect element
		}
	}
	/* Wait for the children to finish */
	
	for (i = 0; cmds[i] != NULL; i++) { /* call wait on all the executed cmds */
		curCmd = cmds[i];
		if (curCmd->pid > 0) { /* an executed process */
			waitpid(curCmd->pid, &curCmd->status, 0);
			if (status == 0) status = curCmd->status; /* gets an error if we had one */
			if (DEBUG) fprintf(stderr,"0-Child %d exited with a %d value\n",curCmd->pid, curCmd->status);
		}
	}

	if (status == 0)
		if (DEBUG) fprintf(stderr, "0-Program Complete\n");
	else
		if (DEBUG) fprintf(stderr, "0-Program Error: %d\n", status);
	return status;
}


rxCommand **rxParse(char *input) {
	char **args;
	rxCommand **cmds = NULL, **scmds;
	
	char *arg, *op = NULL, cchar, pchar = 0, nchar, *env, *tmp, schar, *scmd;
	rxCommand *cmd;
	int i, m, argn, j, argsn, k, cmdsn, l;
	int q = 0, Q = 0, t = 0, e = 0, add = 0, flushc = 0, flusha = 0, error = 0, rtn, len, background = 0, esc = 0, n;

	if (input && *input) {
		cmdsn = 3;
		cmds = (rxCommand**)xmalloc(sizeof(rxCommand*)*cmdsn);
		l = 0;

		argsn = 9; // default number of arguments
		args = (char**)xmalloc(sizeof(char*)*argsn);
		k = 0;

		argn = 50; // default size of argument;
		arg = (char*)xmalloc(sizeof(char)*argn);
		j = 0; //starting point of argument

		for (i = 0; cchar = input[i]; i++) {
			add = 0;
			nchar = input[i+1];	// Will be 0 on the last character

			if (DEBUG) fprintf(stderr, "char(%d): %c, pchar: %c, nchar: %c\n", j, cchar, pchar, nchar);
			
			if (esc) {
				esc = 0;
				add = 1;
			}
			else if (cchar == '\\') { // escape char
				esc = 1;
			}
			else if (q && cchar != '\'') {
				add = 1;
			}
			else if (cchar == '\'') {
				if (DEBUG) fprintf(stderr, "Found single Quote\n");
				if (q == 0 && Q == 0)
					q = 1;
				else if (q == 1 && Q == 0)
					q = 0;
				else
					add = 1;
			}
			else if (cchar == '"') {//it open or closed ?
				if (DEBUG) fprintf(stderr, "Found double Quote\n");
				if (Q == 0 && q == 0)
					Q = 1;
				else if (Q == 1 && q == 0)
					Q = 0;
				else
					add = 1;
			}
			else if (cchar == '|' && !Q) { // found new command
				if (DEBUG) fprintf(stderr, "Found a pipe\n");
				if (!k && !j) {
					fprintf(stderr, "syntax error @ %d: unexpected '|'\n",i+1);
					error = 1;
					break; // go back and see wtf is going on
				}  else {
					flushc = 1;
					op = strdup("|");
				}
			}
			else if (cchar == ' ') { // space found
				if (q) {	//single quotes force us to keep all spaces
					add = 1;
				}
				else if (Q) {	//double quotes can strip them out
					if (pchar != ' ') {
						add = 1;
					}
				}
				else if (j) { //our arg is not blank
					flusha = 1;
				}
			}
			else if (cchar == '$') {
				if (nchar == '$') { // strong quotes so just add
					if (DEBUG) fprintf(stderr, "Strong quote in effect\n");
					//esc = 1;
					add = 1;
					i++;
					cchar = input[i];
				} else {
					for (tmp = input+i+1; *tmp != 0 && ((*tmp >= 'a' && *tmp <= 'z') || (*tmp >= 'A' && *tmp <= 'Z') || (*tmp >= '0' && *tmp <= '0') || *tmp == '_'); tmp++);
					m = tmp - (input+i+1);
					if (m) { // get variable name
						tmp = (char*)xmalloc(sizeof(char)*m+1);
						memcpy(tmp, input+i+1, m);
						tmp[m] = 0;
						env = getenv(tmp);
						if (env) { // we got something back
							if (DEBUG) fprintf(stderr, "env: %s\n",env);
							len = strlen(env);
							if (j+len >= argn-1) { //current arg is bigger then our buffer
								argn += argn/2 + len;
								if (DEBUG) fprintf(stderr, "Making arg buffer bigger %d\n", argn);
								arg = (char*)xrealloc(arg, sizeof(char)*argn);
							}
							memcpy(arg+j, env, len);
							j += len;
						}
						i += m;
						cchar = input[i];
						if (input[i+1] == 0) {
							flushc = 1;
						}
						free(tmp);
					} else {
						add = 1;
					}
					if (DEBUG) fprintf(stderr, "Env Var Len: %d\n", m);
				}
			}
			else if (cchar == '<' && !Q) {
				if (DEBUG) fprintf(stderr, "Found a <\n");
				if (!k && !j) {
					fprintf(stderr, "syntax error @ %d: unexpected '<'\n",i+1);
					error = 1;
					break; // go back and see wtf is going on
				} else {
					flushc = 1;
					op = strdup("<");
				}
			}
			else if (cchar == '>'  && !Q) {
				if (pchar != '>' && nchar != '>') {
					if (DEBUG) fprintf(stderr, "Found a >\n");
					if (!k && !j) {
						fprintf(stderr, "syntax error @ %d: unexpected '>'\n",i+1);
						error = 1;
						break; // go back and see wtf is going on
					}  else {
						flushc = 1;
						op = strdup(">");
					}
				}
				else if (pchar == '>') {
					if (DEBUG) fprintf(stderr, "Found a >>\n");
					if (!k && !j) {
						fprintf(stderr, "syntax error @ %d: unexpected '>>'\n",i+1);
						error = 1;
						break; // go back and see wtf is going on
					}  else {
						flushc = 1;
						op = strdup(">>");
					}
				}
			}
			else if (cchar == '&') {
				if (DEBUG) fprintf(stderr, "Found a &\n");
				if (!k && !j) {
					fprintf(stderr, "syntax error @ %d: unexpected '&'\n",i+1);
					error = 1;
					break; // go back and see wtf is going on
				}  else {
					flushc = 1;
					op = strdup("&");
					background = i+1;
				}
			}
			else if (cchar == '`') {	//Execute command
				if (rxCmd_Sub(input+i+1, &scmd, &n) != -1) {
					if (DEBUG) fprintf(stderr, "cmd: %s\n", scmd);
					i += n+1;
					if (input[i+1] == 0) {
						flushc = 1;
					}
					if (DEBUG) fprintf(stderr, "n: %d\n", i);
					if (DEBUG) fprintf(stderr, "Rest of command: %s\n",input+i+1);
					scmds = rxParse(scmd);
					if (scmds) {
						free(scmd);
						n = rxCmd_ToString(scmds, &scmd);
						if (n >= 0) {
							if (DEBUG) fprintf(stderr, "output: '%s'\n", scmd);
							if (j+n >= argn-1) { //current arg is bigger then our buffer
								argn += argn/2 + n;
								if (DEBUG) fprintf(stderr, "Making arg buffer bigger %d\n", argn);
								arg = (char*)xrealloc(arg, sizeof(char)*argn);
							}
							memcpy(arg+j, scmd, n);
							j += n;
							if (DEBUG) fprintf(stderr, "NEW ARG: %s\n", arg);
							free(scmd);
							loopArray((void**)scmds, (void*)&rxCmd_Free);
							free(scmds);
						}
					} else {
						free(scmd);
						error = 1;
						break; // go back and see wtf is going on
					}
				} else { 
					error = 1;
					break; 
				}
			} else {
				add = 1;
			}

			if (!nchar) { //end of line
				flushc = 1; 
			}

			if (add) { // add current character to argument
				if (background) {
					printf("syntax error @ %d: '&' must be the last command\n", background);
					error = 1;
					break;
				}
				if (j >= argn-1) { //current arg is bigger then our buffer
					if (DEBUG) fprintf(stderr, "Making arg buffer bigger\n");
					argn += 1+argn/2;
					arg = (char*)xrealloc(arg, sizeof(char)*argn);
				}
				arg[j++] = cchar;
			}
			if ((flusha || flushc) && j > 0) {
				if (j < argn-1)		//proporly resize our arg
					arg = (char*)xrealloc(arg, sizeof(char)*(j+1));
				arg[j] = 0;

				if (DEBUG) fprintf(stderr, "Arg found: %s\n\n",arg);
				if (k >= argsn-1) { //number of args is bigger then our buffer
					argsn += 1+argsn/2;
					if (DEBUG) fprintf(stderr, "resizing args to: %d\n",argsn);
					args = (char**)xrealloc(args, sizeof(char*)*argsn);
				}
				args[k++] = arg;

				argn = 50;
				arg = (char*)xmalloc(sizeof(char)*argn);
				j = 0; //starting point of new argument

				flusha = 0;
			}
			if (flushc) {
				if (DEBUG) fprintf(stderr, "Flushing command\n");
				if (q || q) {
					fprintf(stderr, "syntax error: missing quote\n");
				}
				if (k < argsn-1)	//proprly resize our args array
					args = (char**)xrealloc(args, sizeof(char*)*(k+1));
				args[k] = NULL;

				cmd = (rxCommand*)xmalloc(sizeof(rxCommand));
				cmd->id = l;
				cmd->pid = -1;
				cmd->args = args;
				cmd->op = (op ? op : strdup(""));
				cmd->fd[0] = -1;
				cmd->fd[1] = -1;
				cmd->fn = NULL;
				if (DEBUG) rxCmd_Print(cmd);
				if (l >= cmdsn-1) { //number of commands is bigger then our buffer
					cmdsn += 1+cmdsn/2;
					if (DEBUG) fprintf(stderr, "resizing cmds to: %d\n",cmdsn);
					cmds = (rxCommand**)xrealloc(cmds, sizeof(rxCommand*)*cmdsn);
				}
				cmds[l++] = cmd;
				
				argsn = 9; // default number of arguments
				args = (char**)xmalloc(sizeof(char*)*argsn);
				k = 0;
				op = NULL;

				flushc = 0;
			}
			
			pchar = cchar;
		}
		if (error || q || Q || (cmd && cmd->op && *(cmd->op) != 0 && strcmp(cmd->op, "&") != 0) ) {	//we had a syntax error
			if (q)
				fprintf(stderr, "syntax error: expecting '''\n");
			else if (Q)
				fprintf(stderr, "syntax error: expecting '\"'\n");
			else if (cmd && cmd->op && *(cmd->op) != 0 && strcmp(cmd->op, "&") != 0)
				fprintf(stderr, "syntax error: unexpected '%s'\n", cmd->op);

			free(arg);
			if (op) free(op);
			for (i = 0; i < k; i++)
				free(args[i]);
			free(args);
			
			for (i = 0; i < l; i++)
				rxCmd_Free(cmds[i]);
			free(cmds);
			return(NULL);
		}

		if (l < cmdsn-1)	//proprly resize our cmds array
			cmds = (rxCommand**)xrealloc(cmds, sizeof(rxCommand*)*(l+1));
		cmds[l] = NULL;

		//fprintf(stderr, "done\n\n");
		
		//loopArray((void**)cmds, (void*)&rxCmd_Print);
		//rtn = rxCmd_Execute(cmds);
		//loopArray((void**)cmds, (void*)&rxCmd_Free);
	}
	return cmds;
}

char *rxExpandVars(char *input) {
	char *arg = NULL, cchar, pchar = 0, nchar, *env, *tmp;

	int i, m, argn, j;
	int q = 0, Q = 0, add = 0, rtn, len;

	if (input && *input) {

		argn = 100; // default size of argument;
		arg = (char*)xmalloc(sizeof(char)*argn);
		j = 0; //starting point of argument

		for (i = 0; cchar = input[i]; i++) {
			add = 0;
			nchar = input[i+1];	// Will be 0 on the last character
			if (cchar == '"') {//it open or closed ?
				if (DEBUG) fprintf(stderr, "Found double Quote\n");
				if (Q == 0 && q == 0)
					Q = 1;
				else if (Q == 1 && q == 0)
					Q = 0;
				else
					add = 1;
			}
			else if (cchar == '\'') {
				if (DEBUG) fprintf(stderr, "Found single Quote\n");
				if (q == 0 && Q == 0)
					q = 1;
				else if (q == 1 && Q == 0)
					q = 0;
				else
					add = 1;
			}
			else if (cchar == '$') {
				if (q || nchar == '$') { // strong quotes so just add
					add = 1;
				} else if (pchar != '$') {
					for (tmp = input+i+1; *tmp != 0 && ((*tmp >= 48 && *tmp <= 57) || (*tmp >= 97 && *tmp <= 122) || (*tmp >= 65 && *tmp <= 90)); tmp++);
					m = tmp - (input+i+1);
					if (m) { // get variable name
						tmp = (char*)xmalloc(sizeof(char)*m+1);
						memcpy(tmp, input+i+1, m);
						tmp[m] = 0;
						env = getenv(tmp);
						if (env) { // we got something back
							if (DEBUG) fprintf(stderr, "env: %s\n",env);
							len = strlen(env);
							if (j+len >= argn-1) { //current arg is bigger then our buffer
								argn += argn/2 + len;
								if (DEBUG) fprintf(stderr, "Making arg buffer bigger %d\n", argn);
								arg = (char*)xrealloc(arg, sizeof(char)*argn);
							}
							memcpy(arg+j, env, len);
							j += len;
						}
						i += m;
						free(tmp);
					} else {
						add = 1;
					}
					if (DEBUG) fprintf(stderr, "Env Var Len: %d\n", m);
				}
			} else {
				add = 1;
			}

			if (add) { // add current character to argument
				if (j >= argn-1) { //current arg is bigger then our buffer
					if (DEBUG) fprintf(stderr, "Making arg buffer bigger\n");
					argn += 1+argn/2;
					arg = (char*)xrealloc(arg, sizeof(char)*argn);
				}				
				arg[j++] = cchar;
			}
		}
		if (j < argn-1)		//proporly resize our arg
			arg = (char*)xrealloc(arg, sizeof(char)*(j+1));
		arg[j] = 0;
	}
	return arg;
}


int rxCmd_Sub(char *input, char **cmd, int *i) {
	if (!input || !input[0] || !cmd || !i)
		return -1;
	char cchar;
	int cmdn, n;
	int esc = 0, add, error = 0;
	
	cmdn = 50; // default size of command
	*cmd = (char*)xmalloc(sizeof(char)*cmdn);
	n = 0; //starting point of command
	for (*i = 0; cchar = input[(*i)]; (*i)++) {
		if (DEBUG) fprintf(stderr, "cchar: %c, n: %d\n",cchar, n);
		add = 0;
		if (esc) {
			esc = 0;
			add = 1;
		}
		else if (cchar == '\\') { // escape char
			esc = 1;
		}
		else if (cchar == '`') { // found closing tick
			break;
		} else {
			add = 1;
		}
		
		if (add) { // add current character to argument
			if (n >= cmdn-1) { //current cmd is bigger then our buffer
				cmdn += 1+cmdn/2;
				if (DEBUG) fprintf(stderr, "reallocating: %d\n", cmdn);
				*cmd = (char*)xrealloc(*cmd, sizeof(char)*cmdn);
				if (DEBUG) fprintf(stderr, "done allocating\n");
			}
			(*cmd)[n++] = cchar;	//ArghA!!!!! wow that one sucked
		}
	}
	if (DEBUG) fprintf(stderr, "done\n");
	if (cchar != '`') {
		fprintf(stderr, "syntax error: missing closing '`'\n");
		free(*cmd);
		return -1;
	}

	if (DEBUG) fprintf(stderr, "n : %d\n", n);
	if (n < cmdn-1)		//proporly resize our cmd
		*cmd = (char*)xrealloc(*cmd, sizeof(char)*(n+1));
	(*cmd)[n] = 0;

	return n;
}

int rxCmd_ToString(rxCommand **cmds, char **buf) {

	int fd[2], pid;

	if (!cmds || !cmds[0])
		return -1;
		
	if (pipe(fd) == -1) { /* Create pipe*/
		perror("pipe"); exit(EXIT_FAILURE);
	}

	pid = xfork();

	if (pid == 0) { // we are the child
		xclose(fd[0]); // close input side of pipe
		xdup2(fd[1], 1); // set stdout to output side of pipe
		exit(rxCmd_Execute(cmds));
	} else { // we are the parent
		xclose(fd[1]); // close output side of pipe

		int n = 0, bufn = 100, r;
		*buf = (char*)xmalloc(sizeof(char)*bufn);
		while ((r = read(fd[0], (*buf)+n, bufn-1-n)) > 0) {
			n += r;
			if (n >= bufn-1) { //current cmd is bigger then our buffer
				bufn += 1+bufn/2;
				*buf = (char*)xrealloc(*buf, sizeof(char)*bufn);
			}
		}
		xclose(fd[0]); // close input side of pipe
		n--; // get rid of the new line at the end
		if (n < bufn-1)		//proporly resize our cmd
			*buf = (char*)xrealloc(*buf, sizeof(char)*(n+1));
		(*buf)[n] = 0;
		return n;
	}
}
