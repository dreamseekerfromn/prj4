#ifndef PCB_H
#define PCB_H

#define MAXP 20
#define true 1
#define false 0
#define NEW 0
#define READY 1
#define RUNNING 2
#define BLOCK 3
#define TERMINATE 4

typedef struct pcb_ {
	int pid [MAXP];			//store pid
	int flag[MAXP];			//indicate the status of the process
	int turn;
	int num_proc;			//total number of processes 
	int term_proc;
	int r_process;			//remaining processes to fork

	int priority[MAXP];		//priority for MFQ
	int queue[MAXP];		//queue array
	int front;			//queue front
	int rear;			//queue rear


	//storage for times
	int quantum;			//quantum 
	int wait_t[MAXP];		//total waiting time
	int remain_t[MAXP];		//total remaining job
	int turnaround_t[MAXP];		//turnaround time
	int last_burst[MAXP];		//length of the last burst

	int rflag;		//read flag, indicate the oss read the time or not
}pcb;

typedef struct osc_{
	int sec;
	int nsec;
}osc;

#define max(X,Y) ((X) < (Y) ? (X) : (Y))

#endif /* PCB_H */
