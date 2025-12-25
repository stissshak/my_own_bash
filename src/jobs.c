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

job_t *create_job(pid_t pgid, pid_t *pids, size_t n, char *name, job_st status){
	job_t *buf = malloc(sizeof(job_t));
	if(!buf){
		perror("create_job");
		return NULL;
	}
	buf->command = strdup(name);
	if(!buf->command){
		perror("create_job:");
		free(buf);
		return NULL;
	}
	buf->pgid = pgid;
	buf->status = status;
	buf->next = NULL;
	buf->prev = NULL;
	buf->n_pids = n;
	buf->pids = malloc(n * sizeof(pid_st));
	if(!buf->pids){
		perror("create_job");
		free(buf->command);
		free(buf);
		return NULL;
	}
	for(size_t i = 0; i < n; ++i){
		buf->pids[i].pid = pids[i];
		buf->pids[i].status = status;
	}
	buf->job_id = last_job++;
	return buf;
}

int add_job(job_t *new){
	if(!job_list){
		job_list = new;	
		if(job_list) return job_list->job_id;
		return -1;
	}

	job_t *buf = job_list;
	while(buf->next) buf = buf->next;
	buf->next = new;
	if(!buf->next) return -1;
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
	free(buf->command);
	free(buf->pids);
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
		job_t *next = j->next;
		int running = 0, stopped = 0;
		for(size_t i = 0; i < j->n_pids; ++i){
			ret = waitpid(j->pids[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
			if (ret > 0) {
				if (WIFSTOPPED(status)) j->pids[i].status = STOPPED;
				else if (WIFCONTINUED(status)) j->pids[i].status = RUNNING;
				else if (WIFEXITED(status) || WIFSIGNALED(status)) j->pids[i].status = DONE;
			}
		
			if(j->pids[i].status == RUNNING) running = 1;
			if(j->pids[i].status == STOPPED) stopped = 1;

		}

		if(!running && !stopped){
			printf("\n[%d] Done\t%s\n", j->job_id, j->command);
			remove_job(j->job_id);
		} 
		else if(running) j->status = RUNNING;
		else if(stopped) j->status = STOPPED;
		j = next;
	}
}


int jobs(int argc, char *argv[]){
	(void)argc; (void)argv;
	update_jobs();
	if(!job_list){
		printf("jobs: There are no jobs\n");
		return 0;
	}
	printf("Job\tGroup\tState\tCommand\n");
	for(job_t *i = job_list; i; i = i->next){
		char *status = "unknown";
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

static job_t *f_last_stopped(){
    job_t *f = NULL;
    for(job_t *j = job_list; j; j = j->next){
        if(j->status == STOPPED){
            if(!f || j->job_id > f->job_id) f = j;
        }
    }
    return f;
}

int fg(int argc, char *argv[]){
	job_t *job;
    if(argc < 2){
		job = f_last_stopped();
        if(!job){
            fprintf(stderr, "fg: no stopped jobs\n");
		    return 1;
        }
	}
    else{
    	int job_id = atoi(argv[1]);
    	job = find_job(job_id);
    	if(!job){
    		fprintf(stderr, "fg: job %d not found\n", job_id);
    		return 1;
    	}
    }
	tcsetpgrp(STDIN_FILENO, job->pgid);

	if(job->status == STOPPED){
		kill(-job->pgid, SIGCONT);
		for(size_t i = 0; i < job->n_pids; ++i){
			if(job->pids[i].status == STOPPED){
				job->pids[i].status = RUNNING;
			}
		}
		job->status = RUNNING;
	}
	
	int status;

	for(size_t i = 0; i < job->n_pids; ++i){
		if(job->pids[i].status == DONE) continue;

		waitpid(job->pids[i].pid, &status, WUNTRACED);

		if(WIFSTOPPED(status)) job->pids[i].status = STOPPED;
		else job->pids[i].status = DONE;
	}

	tcsetpgrp(STDIN_FILENO, getpgrp());

	int running = 0, stopped = 0;
	for(size_t i = 0; i < job->n_pids; ++i){
		if(job->pids[i].status == RUNNING) running = 1;
		if(job->pids[i].status == STOPPED) stopped = 1;
	}

	if(!running && !stopped){
		remove_job(job->job_id);
	}
	else if(stopped){
		job->status = STOPPED;
		printf("\n[%d] Stopped\t%s\n", job->job_id, job->command);
	}
	else job->status = RUNNING;
	
	if(WIFEXITED(status)) return WEXITSTATUS(status);
	if(WIFSIGNALED(status)) return 128 + WTERMSIG(status);
	if(WIFSTOPPED(status)) return 128 + WSTOPSIG(status);
	return 0;
}

int bg(int argc, char *argv[]){
    job_t *job;
    if(argc < 2){
		job = f_last_stopped();
        if(!job){
            fprintf(stderr, "bg: no stopped jobs\n");
		    return 1;
        }
	}
    else{
    	int job_id = atoi(argv[1]);
    	job = find_job(job_id);
    	if(!job){
    		fprintf(stderr, "bg: job %d not found\n", job_id);
    		return 1;
    	}
    }
	if(job->status == STOPPED){
		kill(-job->pgid, SIGCONT);
		for(size_t i = 0; i < job->n_pids; ++i){
			if(job->pids[i].status == STOPPED){
				job->pids[i].status = RUNNING;
			}
		}
		job->status = RUNNING;
		printf("[%d] %s &\n", job->job_id, job->command);
	}

	return 0;
}

int bkill(int argc, char *argv[]){
	if(argc < 2){
		fprintf(stderr, "kill: need job id\n");
		return 1;
	}

	int sig = SIGTERM;
	int arg_start = 1;

	if(argv[1][0] == '-'){
		sig = atoi(argv[1] + 1);
		if(sig == 0) sig = SIGTERM;
		arg_start = 2;
	}

	if(argc <= arg_start){
		fprintf(stderr, "kill: need pid or %%job\n");
		return 1;
	}

	if(argv[arg_start][0] == '%'){
		int job_id = atoi(argv[arg_start] + 1);
		job_t *job = find_job(job_id);
		if(!job){
			fprintf(stderr, "kill: job %d not found\n", job_id);
			return 1;
		}
		kill(-job->pgid, sig);
	}else{
		pid_t pid = atoi(argv[arg_start]);
		kill(pid, sig);
	}
	return 0;


}

