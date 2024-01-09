#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "os.h"

OS os;

// typedef struct {
// 	int core;
// } scheduler_args;

void schedulerSJF(OS* os, void* args_){
	//scheduler_args* args = (scheduler_args*)args_;

	if (! os->ready.first) {
		printf("AA. Scheduler: ! os->ready.first\n");
		return;
	}

	ListItem* item = os->ready.first;
	PCB* p = (PCB*) item;
	printf("BB. Scheduler: ready.first: %d\n", p->pid);

	PCB* pcb = (PCB*) malloc(sizeof(PCB));
	pcb = shortestJobPCB(item);

	List_detach( &(os->ready), (ListItem*) pcb);

	//  *** aggiunta pcb a running
	List_pushBack( &(os->running), (ListItem*) pcb);

	// PCB* pcb = (PCB*) List_popFront(&os->ready);
	
	
	assert(pcb->events.first);
	ProcessEvent* e = (ProcessEvent*)pcb->events.first;
	assert(e->type==CPU && "AAA");


	if ( e->duration > e->prediction ) { 
		
		ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
		qe->list.prev = 0;
		qe->list.next = 0;
		qe->type = CPU;

		qe->duration = e->prediction;
		qe->prediction = e->next_prediction;
		qe->next_prediction = e->next_prediction;

		e->duration = e->duration - e->prediction;
		e->prediction = e->next_prediction;
		// e->next_prediction = e->next_prediction;

		List_pushFront(&pcb->events, (ListItem*)qe); // aggiunge in testa
	}
};

PCB* shortestJobPCB (ListItem* item){

	if(!item) return NULL;

	PCB* pcb = (PCB*) item;
	ProcessEvent* e = (ProcessEvent*) pcb->events.first;
	assert(e->type==CPU && "BBB");

	PCB* next_pcb = shortestJobPCB(item->next);
	if(!next_pcb) return pcb;

	ProcessEvent* next_e = (ProcessEvent*) next_pcb->events.first;
	if( e->prediction <= next_e->prediction ) return pcb;
	else return next_pcb;

};



int main(int argc, char** argv) {
	
	// *** inserire controlli numero di argomenti

	int core = atoi(argv[1]);
	
	OS_init(&os);
	scheduler_args srr_args;
	srr_args.core = core;
	os.schedule_args = &srr_args;
	os.schedule_fn = schedulerSJF;
	
	

	for (int i=2; i<argc; ++i){
		Process new_process;
		int num_events = Process_load(&new_process, argv[i]);
		Process_CalculatePrediction(&new_process);

		printf("loading [%s], pid: %d, events:%d", argv[i], new_process.pid, num_events);
		if (num_events) {
			Process* new_process_ptr = (Process*)malloc(sizeof(Process));
			*new_process_ptr = new_process;
			List_pushBack(&os.processes, (ListItem*)new_process_ptr); // inserisce alla fine
		}
	}
	printf("num processes in queue %d\n", os.processes.size);
	while(os.running.first || os.ready.first || os.waiting.first || os.processes.first) {
		OS_simStep(&os);
	}
}
