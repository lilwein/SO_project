#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>

#include "aux.h"
#include "os.h"



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