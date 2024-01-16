#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>

#include <termios.h>
#include <sys/types.h>
#include <sys/time.h>

#include "aux.h"
#include "os.h"

ListItem* List_find_process(ListHead* head, ListItem* item) {
	ListItem* aux = head->first;
	int pid_item = ((Process*) item)->pid;
	while(aux){
		Process* p = (Process*) aux;
		if (p->pid==pid_item)
			return item;
		aux=aux->next;
	}
  return 0;
};

void setZeroUsed(ListHead* head){
	ListItem* item = head->first;
	while(item){
		PCB* pcb = (PCB*) item;
		pcb->usedThisTime = 0;
		item = item->next;
	}
};

void printPidList_AUX(ListHead* head, char* name, int n){
	ListItem* item = head->first;
	if(n==-1) printf("%s:\t", name);
	else printf("%s %d:\t", name, n);
	if(!item){
		printf(" -\n");
		return;
	}
	while(item){
		PCB* pcb = (PCB*) item;
		printf("(%d) ", pcb->pid);
		item = item->next;
	}
	printf("\n");
};
void printPidListsDebug(OS* os, int n){
	printf("+++++ LISTS %d +++++\n", n);
	printPidList_AUX(&os->running, "running", n);
	printPidList_AUX(&os->ready, "ready   ", n);
	printPidList_AUX(&os->waiting, "waiting", n);
	printUsed(os, "usedThisTime", n);
	printf("\n");
};

void printPidLists(OS* os){
	printPidList_AUX(&os->running, "running", -1);
	printPidList_AUX(&os->ready, "ready   ", -1);
	printPidList_AUX(&os->waiting, "waiting",-1);
	// printPidList_AUX(&os->processes, "processes",-1);
	printf("\n");
};

int printUsed_AUX(ListHead* head){
	int r = 0;
	ListItem* item = head->first;
	while(item){
		PCB* pcb = (PCB*) item;
		if(pcb->usedThisTime == 1){
			printf("(%d) ", pcb->pid);
			r = 1;
		}
		item = item->next;
	}
	return r;
};
void printUsed(OS* os, char* name, int n){
	printf("%s %d:\t", name, n);
	if (	printUsed_AUX(&os->running) +
			printUsed_AUX(&os->waiting) +
			printUsed_AUX(&os->ready) == 0
	) printf("-");
};

void printEscape(char* str){
	#ifdef _NO_ANSI
		return;
	#endif
	printf("\e[%sm", str);
}




void changemode(int dir){
  	static struct termios oldt, newt;

  	if ( dir == 1 ){
		tcgetattr( STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~( ICANON | ECHO );
		tcsetattr( STDIN_FILENO, TCSANOW, &newt);
	}
	else
		tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

int kbhit (void){
	struct timeval tv;
	fd_set rdfs;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&rdfs);
	FD_SET (STDIN_FILENO, &rdfs);

	select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &rdfs);
}