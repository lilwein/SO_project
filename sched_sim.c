#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "os.h"

OS os;

typedef struct {
	int quantum;
} schedulerSJF_args;

void schedulerSJF(OS* os, void* args_){
	//schedulerSJF_args* args = (schedulerSJF_args*)args_;

	if (! os->ready.first) return;

	ListItem* item = os->ready.first;
	PCB* pcb = (PCB*) malloc(sizeof(PCB));
	pcb = shortestJobPCB(item);

	List_detach( &(os->ready), (ListItem*) pcb);

	// PCB* pcb = (PCB*) List_popFront(&os->ready);
	os->running = pcb;
	
	assert(pcb->events.first);
	ProcessEvent* e = (ProcessEvent*)pcb->events.first;
	assert(e->type==CPU);


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
	assert(e->type==CPU);

	PCB* next_pcb = shortestJobPCB(item->next);
	if(!next_pcb) return pcb;

	ProcessEvent* next_e = (ProcessEvent*) next_pcb->events.first;
	if( e->prediction <= next_e->prediction ) return pcb;
	else return next_pcb;

};



int main(int argc, char** argv) {
	OS_init(&os);
	schedulerSJF_args srr_args;
	srr_args.quantum = 5;
	os.schedule_args = &srr_args;
	os.schedule_fn = schedulerSJF;
	
	for (int i=1; i<argc; ++i){
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
	while(os.running
				|| os.ready.first
				|| os.waiting.first
				|| os.processes.first){
		OS_simStep(&os);
	}
}
