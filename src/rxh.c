#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <sys/dir.h>
#include <unistd.h>
#include <sys/stat.h>
#include "rxCommand.h"
#include "wrappers.h"
#include "rxLib.h"
#include "rxFunctions.h"
#include "rxJobs.h"
#ifndef DEBUG
	#define DEBUG 0
#endif
extern char **environ;
extern int alphasort();

/* A static variable for holding the line. */
static char *line_read = (char *)NULL;
static int lastrtn = 0;
rxJobs *jobs;

/* Read a string, and return a pointer to it.
   Returns NULL on EOF. */
char *rl_gets (char *prompt) {
	/* If the buffer has already been allocated,
	   return the memory to the free pool. */
  if (line_read) {
      free (line_read);
      line_read = (char *)NULL;
    }

	/* Get a line from the user. */
	line_read = readline (prompt);

	/* If the line has any text in it,
	   save it on the history. */
	if (line_read && *line_read)
		add_history (line_read);
	return (line_read);
}

/* struct to store internal command pointers */
typedef struct {
  char *name;			/* User printable name of the function. */
  rl_icppfunc_t *func;	/* Function to call to do the job. */
  char *doc;			/* Documentation for this function.  */
} COMMAND;

COMMAND commands[] = {
	{ "cd", rxf_cd, "Change directory" },
	{ "ls", rxf_ls, "list directory" },
	{ "enviorn", rxf_enviorn, "print enviornmental variables" },
	{ "echo", rxf_echo, "echo" },
	{ "export", rxf_export, "set enviornmental variable" },
	{ "exit", rxf_exit, "exit" },
	{ "jobs", rxf_jobs, "jobs" },
	{ (char *)NULL, (rl_icppfunc_t *)NULL, (char *)NULL }
};

int file_select(const struct direct *entry) {
	if ((strcmp(entry->d_name, ".")== 0) || (strcmp(entry->d_name, "..") == 0))
		return (false);
	return !access(entry->d_name, X_OK);
}

COMMAND *find_command (char *name) {
  unsigned int i;
  for (i = 0; commands[i].name; i++)
    if (strcmp (name, commands[i].name) == 0)
      return (&commands[i]);

  return ((COMMAND *)NULL);
}

char * command_generator (const char *text, int state) {
	static int list_index, len, offset;
	static char **cmds = NULL;
	
	struct direct **files = NULL;
	char *name, *tmp, *cwd;
	int i, j, count, n;

	/* If this is a new word to complete, initialize now.  This includes
	   saving the length of TEXT for efficiency, and initializing the index
	   variable to 0. */
	if (!state) {
		list_index = 0;
		offset = 0;

		if (cmds != NULL) {
			for (i = 0; cmds[i] != NULL; i++)
				free(cmds[i]);
			free(cmds);
		}

		tmp = rindex(text,'/');
		if (tmp != NULL) {	/* scan the cwd */
			offset = tmp-text+1;
			cwd = xcalloc(offset, sizeof(char));
			strncpy(cwd, text, offset-1);

			count = scandir(cwd, &files, file_select, alphasort);
			cmds = (char**)xcalloc(count+1, sizeof(char*));

			for (i = 0; i < count; i++) {
				cmds[i] = strdup(files[i]->d_name);
				free(files[i]);
			}
			free(files);
			free(cwd);
		} else { /* scan the path */
			//printf("Scanning path\n");
			n = 1500; /* Guess at 1500 commands default */
			cmds = (char**)xmalloc(sizeof(char*)*n);

			for (i = 0; commands[i].name != NULL; i++) {
				cmds[i] = strdup(commands[i].name);
			}

			tmp = strdup(getenv("PATH"));
			cwd = strtok(tmp , ":");
			while( cwd != NULL ) {
			    //printf( "result is \"%s\"\n", cwd );
				count = scandir(cwd, &files, 0, alphasort);
				if (count < 0) {
					//perror("scandir");
				} else {
					//printf("found: %d\n",count);
					if (count + i >= n) { /* we need more space */
						n = n + count*2;
						//printf("\n\nAllocating more space: %d\n\n", n);
						cmds = xrealloc(cmds, sizeof(char*)*n);
					}
					for (j = 0; j < count; j++) {
						//printf("Found(%d,%d): %s\n",i,n,files[j]->d_name);
						cmds[i++] = strdup(files[j]->d_name);
						free(files[j]);
					}
					free(files);
				}
				cwd = strtok( NULL, ":");
			}
			free(tmp);
			n = i;
			//printf("\n\nresizing array: %d\n\n", n);
			cmds = xrealloc(cmds, sizeof(char*)*n);
			cmds[n-1] = NULL;
		}
		len = strlen (text+offset);
		//printf("text: %s\n", text+offset);
		//printf("len: %d\n", len);
	}

	/* Return the next name which partially matches from the command list. */
	while (name = cmds[list_index]) {
		list_index++;

		if (strncmp (name, text+offset, len) == 0) {
			tmp = xcalloc(offset+strlen(name)+1, sizeof(char));
			strncpy(tmp, text, offset);
			strcpy(tmp+offset, name);
			return tmp;
		}
	}

	/* If no names matched, then return NULL. */
	return ((char *)NULL);
}

char *environment_generator (const char *text, int state) {
	static int list_index, len, envlen;
	char *tmp, *env;
	if (!state) {
		list_index = 0;
		len = strlen (text);
	}

	while (environ[list_index] != NULL) {
		list_index++;
		tmp = strstr(environ[list_index-1],"=");
		env = xcalloc(tmp - environ[list_index-1] +2, sizeof(char));
		strncpy(env+1, environ[list_index-1], tmp - environ[list_index-1]);
		*env = '$';

		if (strncmp (env, text, len) == 0)
			return env;
		free(env);
	}

	/* If no names matched, then return NULL. */
	return ((char *)NULL);
}

char **command_completion(const char *text, int start, int stop) {
	char **matches = NULL, *cmdSplit;

    /* If this word is at the start of the line, then it is a command
     * to complete. */
     cmdSplit = charrpos(rl_line_buffer, "|`", stop-start);
    if (start == 0 || cmdSplit) {
        rl_completion_append_character = ' ';
        matches = rl_completion_matches(text, command_generator);
    } else if (*text == '$') {
        rl_completion_append_character = ' ';
        matches = rl_completion_matches(text, environment_generator);
    }

    return (matches);
}


void initialize_readline() {
	rl_basic_word_break_characters = " \t\n\"\\'`@><=;|&{(";

	// Tell the completer that we want a crack first.
    rl_attempted_completion_function = command_completion;
}

int main() {

	initialize_readline();
	char *line;
	rxCommand **cmds;
	COMMAND *icmd;
	int i, background;
	jobs = rxJobs_Initialize();
	rxJob *job;

	if (setenv("PS1", "\033[01;31m$USER \033[01;34m$PWD $EMO\033[01;34m#\033[22;37m ", 1)) {
		perror("setenv"); exit(EXIT_FAILURE);
	}
	char *prompt = NULL;
	for (;;) {
		if (prompt)
			free(prompt);
		prompt = rxExpandVars(getenv("PS1"));
		rxJobs_Wait(jobs, 0);
		line = rl_gets(prompt); // read line
		if (!line)
			break;

		if (*line) { /* Not a blank line */
			cmds = rxParse(line);
			lastrtn = -1;
			background = 0;
			if (cmds) {	/* we have some commands */
				if (DEBUG) loopArray((void**)cmds, (void*)&rxCmd_Print);
				for (i = 0; cmds[i]; i++) {
					if(icmd = find_command(cmds[i]->args[0])) {
						//printf("Found Internal Command\n");
						cmds[i]->fn = icmd->func;
					}
					if (strcmp(cmds[i]->op,"&") == 0) {
						background = 1;
						free(cmds[i]->op);
						cmds[i]->op = strdup(""); /* clear the operator so the execute code doesn't freak */
					}
				}
				if (background) {
					//printf("Executing background code\n");
					job = xmalloc(sizeof(rxJob));
					job->status = 1;
					job->cmd = strdup(line);

					job->pid = xfork();
					if (job->pid == 0) { /* we are the child */
						exit(rxCmd_Execute(cmds));
					} else { /* we are the parent */
						rxJobs_Add(jobs, job);
						lastrtn = 0;
					}
				} else {
					lastrtn = rxCmd_Execute(cmds);
				}
				loopArray((void**)cmds, (void*)&rxCmd_Free);
			}
	
			if (lastrtn)
				setenv("EMO", "\033[22;31m:( ", 1);
			else
				setenv("EMO", "\033[22;32m:) ", 1);
		}
	}

    exit(0);
}
