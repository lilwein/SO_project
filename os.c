#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "os.h"

void OS_init(OS* os) {
	os->running = 0;
	
	List_init(&os->ready);
	List_init(&os->waiting);
	List_init(&os->processes);
	
	os->timer = 0;
	os->schedule_fn = 0;
}

void OS_createProcess(OS* os, Process* p) {
	// sanity check
	assert(p->arrival_time==os->timer && "time mismatch in creation");
	// we check that in the list of PCBs there is no
	// pcb having the same pid
	assert( (!os->running || os->running->pid!=p->pid) && "pid taken");

	ListItem* aux = os->ready.first;
	while(aux){
		PCB* pcb = (PCB*)aux;
		assert(pcb->pid!=p->pid && "pid taken");
		aux = aux->next;
	}

	aux = os->waiting.first;
	while(aux){
		PCB* pcb = (PCB*)aux;
		assert(pcb->pid!=p->pid && "pid taken");
		aux = aux->next;
	}

	// all fine, no such pcb exists
	PCB* new_pcb=(PCB*) malloc(sizeof(PCB));
	new_pcb->list.next = 0;
	new_pcb->list.prev = 0;
	new_pcb->pid = p->pid;
	new_pcb->events = p->events;

	assert(new_pcb->events.first && "process without events");

	// depending on the type of the first event
	// we put the process either in ready or in waiting
	ProcessEvent* e = (ProcessEvent*)new_pcb->events.first;

	switch(e->type){
	case CPU:
		List_pushBack(&os->ready, (ListItem*) new_pcb);
		break;
	case IO:
		List_pushBack(&os->waiting, (ListItem*) new_pcb);
		break;
	default:
		assert(0 && "illegal resource");
		;
	}
}




void OS_simStep(OS* os){
	
	printf("\n************** TIME: %08d **************\n", os->timer);

	//scan process waiting to be started
	//and create all processes starting now
	ListItem* aux = os->processes.first;
	while (aux){
		Process* proc=(Process*)aux;
		Process* new_process = 0;
		if (proc->arrival_time == os->timer){
			new_process = proc;
		}
		aux = aux->next;
		if (new_process) {
			printf("\tcreate pid:%d\n", new_process->pid);
			new_process = (Process*) List_detach(&os->processes, (ListItem*)new_process);
			OS_createProcess(os, new_process); // trasformo in pcb
			free(new_process);
		}
	}


	// call schedule, if defined
	if (os->schedule_fn && ! os->running){
		(*os->schedule_fn)(os, os->schedule_args); 
		// lo scheduler modificherà os->running
	}

	// if running not defined and ready queue not empty
	// put the first in ready to run
	if (! os->running && os->ready.first) {
		os->running = (PCB*) List_popFront(&os->ready);
	}




	// scan waiting list, and put in ready all items whose event terminates
	aux = os->waiting.first;
	while(aux) {
		PCB* pcb = (PCB*)aux;
		aux = aux->next;
		ProcessEvent* e = (ProcessEvent*) pcb->events.first;
		printf("\twaiting pid: %d\n", pcb->pid);
		assert(e->type==IO);
		
		e->duration--; // è passata un epoca
		printf("\t\tremaining time:%d\n",e->duration);
		
		if (e->duration == 0){ // evento terminato
			printf("\t\tend burst\n");
			
			List_popFront(&pcb->events); // detach first
			free(e);
			List_detach(&os->waiting, (ListItem*)pcb); // cancello pcb dalla lista waiting
			
			if (! pcb->events.first) { // eventi finiti
				// kill process
				printf("\t\tend process\n");
				free(pcb);
			}
			else { // ancora eventi
				//handle next event
				e = (ProcessEvent*) pcb->events.first;

				switch (e->type){ // sposto pcb in lista running o waiting
				case CPU:
					printf("\t\tmove to ready\n");
					List_pushBack(&os->ready, (ListItem*) pcb);
					break;
				case IO:
					printf("\t\tmove to waiting\n");
					List_pushBack(&os->waiting, (ListItem*) pcb);
					break;
				}
			}
		}
	}



	// decrement the duration of running
	// if event over, destroy event and reschedule process
	// if last event, destroy running
	PCB* running = os->running;
	printf("\trunning pid: %d\n", running?running->pid:-1);

	if (running) {
		ProcessEvent* e = (ProcessEvent*) running->events.first;
		assert(e->type==CPU);
		e->duration--;
		printf("\t\tremaining time:%d\n",e->duration);

		if (e->duration==0){ // abbiamo consumato il burst del processo running
			printf("\t\tend burst\n");
			List_popFront(&running->events);
			free(e);
			if (! running->events.first) {
				printf("\t\tend process\n");
				free(running); // kill process
			}
			else {
				e = (ProcessEvent*) running->events.first;

				switch (e->type){
				case CPU:
					printf("\t\tmove to ready\n");
					List_pushBack(&os->ready, (ListItem*) running);
					break;
				case IO:
					printf("\t\tmove to waiting\n");
					List_pushBack(&os->waiting, (ListItem*) running);
					break;
				}
			}
			os->running = 0;
		}
	}


	

	++os->timer;

}

void OS_destroy(OS* os) {
}
