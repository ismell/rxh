#include "rxJobs.h"

rxJobs *rxJobs_Initialize() {
	rxJobs *jobs = (rxJobs*)xmalloc(sizeof(rxJobs));
	jobs->i = -1;
	jobs->n = 3;
	jobs->cnt = 0;
	jobs->jobs = (rxJob**)xmalloc(sizeof(rxJob*)*jobs->n);
	return jobs;
}

int rxJobs_Add(rxJobs *jobs, rxJob *job){
	if (!jobs || !job)
		return -1;

	if (jobs->i >= jobs->n-1) { //current arg is bigger then our buffer
		if (DEBUG) fprintf(stderr, "Making jobs buffer bigger\n");
		jobs->n += 2;
		jobs->jobs = (rxJob**)xrealloc(jobs->jobs, sizeof(rxJob*)*jobs->n);
	}
	job->id = ++jobs->cnt;
	job->status = 1;
	jobs->jobs[++(jobs->i)] = job;
	printf("[%d] %d\n",job->id, job->pid);
	return 0;
}
void rxJobs_Print(rxJobs *jobs) {
	int i;
	if (!jobs || jobs->i < 0)
		return;
	if (DEBUG) fprintf(stderr, "n: %d, i: %d\n", jobs->n, jobs->i);
	for (i = 0; i <= jobs->i; i++)
		rxJob_Print(jobs->jobs[i]); 

	return;
}
void rxJob_Print(rxJob *job) {
	char *status;
	if (!job) 
		return;
	printf("[%d] %-10s %-10d %s\n", job->id, (job->status ? "running" : "done"), job->pid, job->cmd);

	return;
}

void rxJobs_Wait(rxJobs *jobs, int hang) {
	int i = 0, j;
	rxJob *job;
	if (DEBUG) fprintf(stderr, "n: %d, I: %d\n", jobs->n, jobs->i);

	if (!jobs || jobs->i < 0)
		return;
	for (i = 0; i <= jobs->i; i++) {
		job = jobs->jobs[i];
		if (DEBUG) fprintf(stderr, "   i: %d, I: %d, pid: %d\n", i, jobs->i, job->pid);
		if (job && waitpid(job->pid, &job->rtn, (hang ? 0 : WNOHANG))) {
			job->status = 0;
			rxJob_Print(job);
			free(job);
			if (jobs->i-i > 0) { /* its not the last element, shift the array */
				if (DEBUG) fprintf(stderr, "shifting %d\n", jobs->i-i);
				//wmemmove(jobs->jobs[i], jobs->jobs[i+1], sizeof(rxJob*)*(jobs->i-i)); /* Whoah this one sucked! */
				for (j = i; j < jobs->i; j++) {
					jobs->jobs[j] = jobs->jobs[j+1]; /* God damn it finally!*/
				}
				i--; // we need to loop over the same element again
			}
			jobs->i--;
		}
	}
	if (DEBUG) fprintf(stderr, "final: i) %d, I) %d\n", i, jobs->i);
	return;
}
