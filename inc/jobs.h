// jobs.h

#pragma once

#include <unistd.h>
#include <sys/types.h>


typedef enum {
	RUNNING,
	STOPPED,
	DONE
}job_st;

typedef struct pid_st{
	pid_t pid;
	job_st status;
}pid_st;

typedef struct job_t{
	pid_t pgid;
	char *command;
	pid_st *pids;
	size_t n_pids;
	struct job_t *next, *prev;
	job_st status;
	int job_id;
}job_t;


extern job_t *job_list;

job_t *create_job(pid_t, pid_t *, size_t, char*, job_st);
int add_job(job_t*); // return jobId
int remove_job(int);
job_t *find_job(int);
void update_jobs();
int jobs(int, char*[]);
int fg(int, char*[]);
int bg(int, char*[]);
int bkill(int, char*[]);
