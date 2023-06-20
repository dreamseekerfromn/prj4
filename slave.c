#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "log.h"
#include "pcb.h"

//global var
int shmid;
int turn_id;
int semid;
char *fname = "test.out";
int proc_num;

//prototype for signal handler
void sig_handler(int signo);
void kill_shm();
void kill_sem();
void semlock();
void semunlock();

int 
main(int argc, char *argv[]){
	fflush(stdout);
	//init var
	int c, iflag, lflag, err_flag;
	//char *fname = "test.out";	//for -l switch, default is logfile.txt
	char str[256];			//for strtol
	char *strbuff = "3";		//handling default value for -n
	
	char *endptr;			//for strtol
	long value;			//for strtol

	int pid = getpid();		//for pid
	int sig_num;			//for signal

	int k;				//loop
	int i = atoi(argv[1]);		//will store process #
	int max_writing;		//max writes
	int *num;

	int tmp_quantum;		//tmp for quantum
	int partial_w;
	int l_sec;			//last access - sec
	int l_nsec;			//last access - nano sec
	int i_sec;			//initial access -sec
	int i_nsec;			//initial access -nano
	int p_sec,p2_sec;		//partial burst (just for cal
	int p_nsec,p2_nsec;		//just for calc
	int partial_b;
	int remain_t;			//remaining job
	int ta_start, ta_end;		//turnaround time
	int tmp_t;			//tmp storage

	int option_index = 0;
	opterr = 0;
	iflag = 0;
	lflag = 0;
	err_flag = 0;

	proc_num = i;

	//time info
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	//get nano second
	struct timespec tps, tpe;

	//seed random
	srand(time(NULL) ^ (getpid()<<16) );

	//int shmid;
	osc *o;
	pcb *p;

	signal(SIGINT, sig_handler);	//handling signal

        //using atoi b/c process always pass the right type. free to use atoi w/o additional checking
        max_writing = atoi(argv[3]);
        fname = (char *)malloc(strlen(argv[5])+1);
        strcpy(fname,argv[5]);

	snprintf(str,sizeof(str),"process %d : exec success, now start the process");
	create_log(str);
	memset(str,0,sizeof(str));
	savelog(fname);        

	//create a shared memory
	if ((shmid = shmget((key_t)12348888, sizeof(int), 0600)) == -1)
		{
			perror("process : fail to get a shared memory\n");
			snprintf(str,sizeof(str),"process %d : fail to get a shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : got a shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			clearlog();
			memset(str,0,sizeof(str));
		}

	if ((turn_id = shmget((key_t)88881234, sizeof(int)*20, 0600)) == -1)
		{
			perror("process : fail to get a shared memory for the peterson algorithm\n");
			snprintf(str,sizeof(str),"process %d : fail to get a shared memory for the peterson algorithm\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process%d : get a second shared memory\n", proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();

		}

	//attatching shared memory
	//first shm for data
	if ((o = (osc *)shmat (shmid, NULL, 0)) == -1)
		{
			perror("process : fail to attatch the shared memory\n");
			snprintf(str,sizeof(str),"process %d : fail to attatch the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : success to attatch the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
		}

	//second for the algorithm
	if ((p = (pcb *)shmat(turn_id,NULL,0)) == -1)
		{
			perror("process : fail to attatch the second shared memory\n");
			snprintf(str,sizeof(str),"process %d : fail to attatch the second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : success to attatch the second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
		}

	//semaphore
	if((semid = semget((key_t)11114444,1, 0600)) == -1)
		{
			snprintf(str,sizeof(str),"process %d : fail to create semaphore\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : got the semaphore\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
		}

	//initialize remainder
	remain_t = rand()%10000+1;

	//initialize start time
	clock_gettime(CLOCK_REALTIME, &tps);
	l_sec = o->sec;
	l_nsec = o->nsec;

	ta_start = l_sec * 1000000 + l_nsec;
	p->remain_t[i] = remain_t;
	p->flag[i] = READY;
	fprintf(stderr,"process %d is changed its status (NEW->READY)\n",i);
	snprintf(str,sizeof(str),"process %d : change the status (NEW->READY) at osc time %d sec : %d nano sec (remaining job : %d)\n",i,l_sec,l_nsec, p->remain_t[i]);
	fflush(stdout);
	create_log(str);
	memset(str,0,sizeof(str));
	savelog(fname);
	clearlog();

	while(1)
		{
			//if its status is BLOCK, change to ready and wait
			if (p->flag[i] == BLOCK)
				{
					fprintf(stderr,"process %d is changed its status to READY (BLOCK->READY)\n",i);
					snprintf(str,sizeof(str),"process %d : changed status (BLOCK->READY)\n",i);
					fflush(stdout);
					create_log(str);
					memset(str,0,sizeof(str));
					savelog(fname);
					clearlog();
					p->flag[i] = READY;
					continue;
				}
			if(p->flag[i] == RUNNING)
				{
					fprintf(stderr,"\n process %d is RUNNING\n",i);
					i_nsec = o->nsec;
					p2_nsec = i_nsec;
					i_sec = o->sec;
					p2_sec = i_sec;
					partial_w = (i_sec - l_sec) * 1000000 + (i_nsec - l_nsec);			//wait time for current cycle
					p->wait_t[i] += partial_w;
					snprintf(str,sizeof(str),"process %d : changed status (READY->RUNNING) at osc time %d sec : %d nano sec\n Wait time : %d sec %d nano sec \t Total Wait Time : %d sec %d nano sec\n",i,i_sec,i_nsec,i_sec-l_sec,i_nsec-l_nsec,p->wait_t[i]/1000000,p->wait_t[i]%1000000);
					fflush(stdout);
					create_log(str);
					memset(str,0,sizeof(str));
					savelog(fname);
					clearlog();

					//spending mode, 0 = all in, 1 = partial
					switch(rand()%2)
						{
							case 0:
								fprintf(stderr,"spending entier quantum\n");
								while(p->flag[i] == RUNNING)
									{
										//semlock();
										l_nsec = o->nsec;
										l_sec = o->sec;
										p_nsec = l_nsec;
										p_sec = l_sec;
										partial_b = (p_sec - p2_sec) * 1000000 + (p_nsec - p2_nsec);

										tmp_t = (l_sec - i_sec) * 1000000 + (l_nsec - i_nsec);
										p->remain_t[i] -= partial_b;
										//fprintf(stderr,"process %d \t priority: %d\ttotal burst : %d\t partial burst: %d\tremain job: %d \tp1 = %d:%d \t p2 = %d:%d\tosc = %d:%d\n",i,p->priority[i],tmp_t,partial_b,p->remain_t[i],p_sec,p_nsec,p2_sec,p2_nsec,o->sec,o->nsec);
										if(p->remain_t[i] <= 0)
											{
												p->remain_t[i] = 0;
												fprintf(stderr,"process %d is done its job, terminate\n",i);
												p->flag[i] = TERMINATE;
												ta_end = l_sec * 1000000 + l_nsec;
												p->turnaround_t[i] = ta_end - ta_start;
												snprintf(str,sizeof(str),"process %d : changed its status (RUNNING->TERMINATE) at osc %d sec : %d nano sec\n",i,l_sec,l_nsec);
												fflush(stdout);
												create_log(str);
												memset(str,0,sizeof(str));
												savelog(fname);
												clearlog();
												p->term_proc++;
												//semunlock();
												break;
											}
										else if(tmp_t>=p->quantum)
											{
												fprintf(stderr,"process %d is used all its quantum\n",i);
												snprintf(str,sizeof(str),"process %d : changed its status (RUNNING->READY) at osc %d sec : %d nano sec\n",i,l_sec,l_nsec);
												fflush(stdout);
												create_log(str);
												memset(str,0,sizeof(str));
												savelog(fname);
												clearlog();
												p->flag[i] = READY;
												//semunlock();
												break;
											}
										else
											{
												p2_nsec = l_nsec;
												p2_sec = l_sec;
											//	semunlock();
											}
									
									}
								p->last_burst[i] = tmp_t;
								//p->remain_t[i] -= tmp_t;
								//p->flag[i] = READY;
								p->rflag = 1;
								
								break;
							//partial spending
							case 1:
								fprintf(stderr,"spending partial quantum\n");
								tmp_quantum = rand()%p->quantum;
								while(p->flag[i] == RUNNING)
									{
										//semlock();
										l_nsec = o->nsec;
										l_sec = o->sec;
										p_nsec = l_nsec;
										p_sec = l_sec;
										partial_b = (p_sec - p2_sec) * 1000000 + (p_nsec - p2_nsec);

										tmp_t = (l_sec - i_sec) * 1000000 + (l_nsec - i_nsec);
										//p->remain_t[i] -= partial_b;
										//fprintf(stderr,"process %d \t priority: %d\ttotal burst : %d\t partial burst: %d\tremain job: %d \tp1 = %d:%d \t p2 = %d:%d\tosc = %d:%d\n",i,p->priority[i],tmp_t,partial_b,p->remain_t[i],p_sec,p_nsec,p2_sec,p2_nsec,o->sec,o->nsec);
									
										
										if(p->remain_t <= partial_b)
											{
												fprintf(stderr,"process %d is changed its status (RUNNING->TERMINATE)\n",i);
												p->remain_t[i] = 0;
												p->flag[i] = TERMINATE;
												ta_end = l_sec * 1000000 + l_nsec;
												p->turnaround_t[i] = ta_end - ta_start;
												p->term_proc++;
												
												snprintf(str,sizeof(str),"process %d : changed its status (RUNNING->TERMINATE) at osc %d sec : %d nano sec\n",i,l_sec,l_nsec);
												fflush(stdout);
												create_log(str);
												memset(str,0,sizeof(str));
												savelog(fname);
												clearlog();
											//	semunlock();
												break;
											}
										p2_nsec = l_nsec;
										p2_sec = l_sec;
										p->remain_t[i] -= partial_b;
										
										if(tmp_t >= p->quantum)
											{
												fprintf(stderr,"process %d is changed its status (RUNNING->READY)\n",i);

												snprintf(str,sizeof(str),"process %d : changed its status (RUNNING->READY) at osc %d sec : %d nano sec\n",i,l_sec,l_nsec);
												fflush(stdout);
												create_log(str);
												memset(str,0,sizeof(str));
												savelog(fname);
												clearlog();

												p->flag[i] = READY;
												//semunlock();
												break;
											}
										//semunlock();
										
									}
								p->last_burst[i] = tmp_t;
								//p->remain_t[i] -= tmp_t;
								if(p->remain_t[i] < 0)
									p->remain_t[i] = 0;
								p->rflag = 1;
								break;
						} 	
				}

			if(p->flag[i] == TERMINATE)
				{
					break;
				}
		}

	//terminating, record the result to pcb
	p->flag[i] = TERMINATE;
	p->num_proc--;

	if(shmdt(o) == -1)
		{
			perror("process : shmdt failed\n");
			snprintf(str,sizeof(str),"process %d : shmdt failed\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : shmdt success\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}

	savelog(fname);
	fprintf(stderr,"process %d (pid:%d) is terminated successfully\n\n",proc_num,getpid());
	return 0;
}

//signal handler
void 
sig_handler(int signo)
{
	char str[256];
	fprintf(stderr,"process : receive a signal,terminating\n",proc_num);
	snprintf(str,sizeof(str),"process %d : receive a signal, terminating\n",proc_num);
	fflush(stdout);
	create_log(str);
	memset(str,0,sizeof(str));
	kill_shm();
	kill_sem();
	savelog(fname);
	sleep(1);
	exit (1);
}

//function for killing a shared memory
void 
kill_shm()
{
	char str[256];
	if((shmctl(shmid, IPC_RMID, 0)) == -1)
		{
			fprintf(stderr,"process %d : fail to kill the shared memory\n",proc_num);
			snprintf(str,sizeof(str),"process %d : fail to kill the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"process %d: success to kill the shared memory\n",proc_num);
			snprintf(str,sizeof(str),"process %d : success to kill the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
	if((shmctl(turn_id, IPC_RMID, 0)) == -1)
		{
			fprintf(stderr,"process %d: fail to kill the second shared memory\n",proc_num);
			snprintf(str,sizeof(str),"process %d : fail to kill the second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"process %d: success to kill the second shared memory\n",proc_num);
			snprintf(str,sizeof(str),"process %d : success to kill second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
}

void
semlock()
{
	char str[256];
	struct sembuf semo;
	semo.sem_num = 0;
	semo.sem_op = 1;
	semo.sem_flg = 0;
	if((semop(semid, &semo, 1)) == -1)
		{
			fprintf(stderr,"process %d : fail to lock semaphore\n",proc_num);
			snprintf(str,sizeof(str),"process %d : fail to lock semaphore\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
			exit(1);
		}
	else
		{
                        snprintf(str,sizeof(str),"process %d : success to lock\n",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();

		}
}

void
semunlock()
{
	char str[256];
	struct sembuf semo;
	semo.sem_num = 0;
	semo.sem_op = -1;
	semo.sem_flg = 0;
	if((semop(semid,&semo, 1)) == -1)
		{
			fprintf(stderr,"process %d : fail to unlock semaphore\n",proc_num);
                        snprintf(str,sizeof(str),"process %d : fail to unlock seamphore",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();
			exit(1);
		}
	else
		{
                        snprintf(str,sizeof(str),"process %d : success to unlock semaphore\n",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();
		}
}

void
kill_sem()
{
	char str[256];
	if((semctl(semid, 0, IPC_RMID)) == -1)
                {
			fprintf(stderr,"process %d : fail to kill semaphore\n",proc_num);
                        snprintf(str,sizeof(str),"process %d : fail to kill semaphore\n",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();
                }
        else
                {
                        snprintf(str,sizeof(str),"process %d : success to kill semaphore\n",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();

                }
}
