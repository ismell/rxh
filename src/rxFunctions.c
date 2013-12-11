#include "rxFunctions.h"

extern rxJobs *jobs;
extern char **environ;
extern int alphasort();

int ls_file_select(const struct direct *entry) {
	if ((strcmp(entry->d_name, ".")== 0) || (strcmp(entry->d_name, "..") == 0))
		return 0;
	return 1;
}

int rxf_cd (char ** args) {
	char *ncwd;
	//printf("Changing Directory\n");
	if (!args || args[0] == NULL) {
		fprintf(stderr, "cd: Invalid args\n"); 
		return EXIT_FAILURE;
	} else if (args[1] == NULL) { // switch to home
		ncwd = getenv("HOME");
		if (ncwd) {
			if (chdir (ncwd) == -1) {
		      perror ("chdir");
		      return errno;
		    }
		}
	}
	else {
		if (chdir (args[1]) == -1) {
	      perror ("chdir");
	      return errno;
	    }
	}
	ncwd = (char*)xgetcwd();
	setenv("PWD", ncwd, 1);
	free (ncwd);
	return EXIT_SUCCESS;
}
int rxf_ls (char ** args) {
	if (!args || !args[0])
		return -1;
	
	if (!args[1]) /* no arguments */
		args = (char*[]){args[0],".",NULL};
	
	register int i, j;
	int rtn = 0, count, width = 80, n = 0, len;
	
	struct direct **files = NULL;
	struct winsize win;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0 && win.ws_col > 0)
			width = win.ws_col;

	for (i = 1; args[i] != NULL; i++) {
		count = scandir(args[i], &files, ls_file_select, alphasort);
		if (count == -1) {
			perror("scandir");
			rtn = -1;
		} else {
			if (i > 1) printf("\n");
			printf("%s:\n",args[i]);
			for (j = 0; j < count; j++) {
				/*len = strlen(files[j]->d_name); // we can do terminal width later
				if (n + 11 > width) {
					n = 0;
					printf("\n");
				}
				n += 11;*/
				printf("%s\n", files[j]->d_name);
				free(files[j]);
			}
			free(files);
		}
	}
	return rtn;
}
int rxf_enviorn (char ** args) {
	register int i;
	for(i = 0; environ[i]; i++)
		printf("%s\n", environ[i]);
	return 0;
}
int rxf_echo (char ** args) {
	int i, nl = 1;
	if (!args || !args[0])
		return EXIT_FAILURE;
	for (i = 1; args[i]; i++) {
		if (strcmp(args[i], "-n") == 0)
			nl = 0;
		else
			printf("%s ", args[i]);
	}
	if (nl)
		printf("\n");
	return 0;
}

int rxf_export (char ** args) {
	if (!args || !args[0])
		return -1;

	if (!args[1]) {
		usage: fprintf(stderr, "Usage: %s NAME=value\n", args[0]);
		return -2;
	}

	char *eq, *str;
	str = args[1]; /* just to make life easy */
	if (eq = index(str, '=')) { /* we found the = sign */
		*eq = '\0';
		setenv(str, eq+1, 1);
	} else {
		goto usage;
	}
	return 0;
}
int rxf_jobs (char ** args) {
	rxJobs_Print(jobs, 0);
	return 0;
}
int rxf_exit (char ** args) {
	rxJobs_Wait(jobs, 1);
	exit(0);
}

