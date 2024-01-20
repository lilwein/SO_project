#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "os.h"
#include "scheduler.h"

OS os;

typedef struct {
	int quantum;
} SchedRRArgs;

void schedulerRR(OS* os, void* args_){
	SchedRRArgs* args=(SchedRRArgs*)args_;

	// look for the first process in ready
	// if none, return
	if (! os->ready.first)
		return;

	PCB* pcb=(PCB*) malloc(sizeof(PCB));
	pcb = (PCB*) List_popFront(&os->ready);
	List_pushFront( &(os->running), (ListItem*) pcb);
	
	assert(pcb->events.first);
	ProcessEvent* e = (ProcessEvent*)pcb->events.first;
	assert(e->type==CPU);

	// look at the first event
	// if duration>quantum
	// push front in the list of event a CPU event of duration quantum
	// alter the duration of the old event subtracting quantum
	if (e->duration>args->quantum) { // divido l'evento per il quanto
		ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
		qe->list.prev=qe->list.next=0;
		qe->type=CPU;
		// split
		qe->duration = args->quantum; // primo evento, durata = 1 quanto
		e->duration -= args->quantum; // secondo evento, durata = durata-quanto
		List_pushFront(&pcb->events, (ListItem*)qe); // aggiunge in testa
	}
};
