// jobs.c

#define _POSIX_C_SOURCE 200809L

#include "jobs.h"

#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

job_t *job_list;
int last_job = 1;

job_t *create_job(pid_t pgid, char *name, job_st status){
	job_t *buf = malloc(sizeof(job_t));
	if(!buf){
		perror("create_job");
		return NULL;
	}
	buf->command = strdup(name);
	if(!buf->command){
		perror("create_job:");
		return NULL;
	}
	buf->pgid = pgid;
	buf->status = status;
	buf->next = NULL;
	buf->prev = NULL;
	buf->job_id = last_job++;
	return buf;
}

int add_job(pid_t pgid, char* name, job_st status){
	if(!job_list){
		job_list = create_job(pgid, name, status);	
		return job_list->job_id;
	}

	job_t *buf = job_list;
	while(buf->next) buf = buf->next;
	buf->next = create_job(pgid, name, status);
	buf->next->prev = buf;
	return buf->next->job_id;
}

int remove_job(int job_id){
	job_t *buf = find_job(job_id);
	if(!buf){
		return -1;
	}
	if(buf == job_list) job_list = buf->next;
	if(buf->next) buf->next->prev = buf->prev;
	if(buf->prev) buf->prev->next = buf->next;
	int jid = buf->job_id;
	free(buf);
	return jid;
}

job_t *find_job(int job_id){
	if(!job_list){
		return NULL;
	}

	job_t *buf = job_list;
	while(buf){
		if(buf->job_id == job_id){
			return buf;
		}
		buf = buf->next;
	}
	return NULL;
}

void update_jobs(){
	job_t *j = job_list;
	int status;
	pid_t ret;
	while (j) {
		ret = waitpid(-j->pgid, &status, WNOHANG | WUNTRACED | WCONTINUED);
		if (ret > 0) {
			if (WIFSTOPPED(status)) j->status = STOPPED;
			else if (WIFCONTINUED(status)) j->status = RUNNING;
			else if (WIFEXITED(status) || WIFSIGNALED(status)){
				job_t *buf = j;
				j = j->next;
				printf("\nsash: Job %d, '%s' has ended\n", buf->job_id, buf->command);
				int removed = remove_job(buf->job_id);
				continue;
			}
		}
		j = j->next;
	}
}


int jobs(int argc, char *argv[]){
	update_jobs();
	if(!job_list){
		printf("jobs: There are no jobs\n");
		return 1;
	}
	printf("Job\tGroup\tState\tCommand\n");
	for(job_t *i = job_list; i; i = i->next){
		char *status;
		switch(i->status){
			case(RUNNING):
				status = "running";
				break;
			case(STOPPED):
				status = "stopped";
				break;
			case(DONE):
				status = "done";
				break;
			default:
				break;
		}	
		printf("%d\t%d\t%s\t%s\n", i->job_id, i->pgid, status, i->command);
	}

	return 0;	
}
