// jobs.h

#pragma once

#include <unistd.h>
#include <sys/types.h>


typedef enum {
	RUNNING,
	STOPPED,
	DONE
}job_st;

typedef struct job_t{
	int job_id;
	pid_t pgid;
	char *command;
	struct job_t *next, *prev;
	job_st status;
}job_t;


extern job_t *job_list;


int add_job(pid_t, char*, job_st); // return jobId
int remove_job(int);
job_t *find_job(int);
void update_jobs();
int jobs(int, char*[]);
int fg(int, char*[]);
int bg(int, char*[]);
int bkill(int, char*[]);
