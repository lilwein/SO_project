#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "os.h"

void OS_init(OS* os) {
	
	List_init(&os->running);
	List_init(&os->ready);
	List_init(&os->waiting);
	List_init(&os->processes);
	
	os->timer = 0;
	os->schedule_fn = 0;
}

void OS_createProcess(OS* os, Process* p) {

	// controlliamo se il processo p è arrivato ora
	assert( p->arrival_time == os->timer && "Error on creation: time mismatch");

	// controlliamo che nessun processo in running abbia lo stesso pid del processo p
	ListItem * aux = os->running.first;
	while(aux){
		PCB* pcb = (PCB*) aux;
		assert( pcb->pid != p->pid && "Error in running list: pid taken");
		aux = aux->next;
	}
	
	// controlliamo che nessun processo in ready abbia lo stesso pid del processo p
	aux = os->ready.first;
	while(aux){
		PCB* pcb = (PCB*) aux;
		assert( pcb->pid != p->pid && "Error in ready list: pid taken");
		aux = aux->next;
	}

	// controlliamo che nessun processo in waiting abbia lo stesso pid del processo p
	aux = os->waiting.first;
	while(aux){
		PCB* pcb = (PCB*)aux;
		assert( pcb->pid != p->pid && "Error in waiting list: pid taken");
		aux = aux->next;
	}

	// nessun pcb con pid uguale
	PCB* new_pcb = (PCB*) malloc(sizeof(PCB));
	new_pcb->list.next = 0;
	new_pcb->list.prev = 0;
	new_pcb->pid = p->pid;
	new_pcb->events = p->events;

	// controlliamo che il processo p abbia eventi
	assert( new_pcb->events.first && "Error on creation: process without events");

	// inserimento del pcb in ready o waiting list
	ProcessEvent* e = (ProcessEvent*)new_pcb->events.first;

	switch(e->type){
	case CPU:
		List_pushBack(&os->ready, (ListItem*) new_pcb);
		break;
	case IO:
		List_pushBack(&os->waiting, (ListItem*) new_pcb);
		break;
	default:
		assert(0 && "Error on creation: illegal resource");
		;
	}
}




void OS_simStep(OS* os){
	
	printf("\n************** TIME: %08d **************\n", os->timer);

	// selezione dei processi arrivati al tempo os->timer
	// trasformazione dei processi pronti in pcb 
	// inserimento dei pcb in ready o waiting list
	ListItem* aux = os->processes.first;
	while (aux){
		Process* proc = (Process*) aux;
		Process* new_process = 0;
		if (proc->arrival_time == os->timer){
			new_process = proc;
		}
		aux = aux->next;
		if (new_process) {
			printf("\tCREATE process with pid: (%d)\n", new_process->pid);
			new_process = (Process*) List_detach(&os->processes, (ListItem*)new_process);
			OS_createProcess(os, new_process); // trasformo in pcb
			free(new_process);
		}
	}

	// numero di core disponibili
	scheduler_args* args = (scheduler_args*) os->schedule_args;
	int core = args->core;

	// ripetere per ogni core disponibile
	for(int i=0; i<core; i++){

		// controllo sul numero di processi in running
		int runningProcess = os->running.size;

		assert( runningProcess <= core && "Error: too many process in running" );
		assert( runningProcess >= 0 && "Error: invalid size of running list" );

		// invocare lo scheduler se verificate le seguenti condizioni: 
		// 	1) scheduler è definito;
		//	2) lista processi running non piena.

		// Osservazioni:
		// 	- La capienza massima della lista dei processi in running equivale al numero di core disponibili
		//	- Se la lista dei processi in running è piena, NON viene invocato lo scheduler: la preemption è
		//		gestita dalla durata rimanente degli eventi
		//	- La rimozione di un processo dalla running list è gestita in seguito in questa funzione
		//	- L'aggiunta di un processo alla running list è gestita dallo scheduler 

		if (os->schedule_fn && runningProcess < core){
			(*os->schedule_fn) (os, os->schedule_args);
		}

		// *********************
		// se la running list non è piena e la coda ready non vuota, aggiungiamo
		// il / i processi a running 
		
		// if (! os->running && os->ready.first) {
		// 	os->running = (PCB*) List_popFront(&os->ready);
		// }



		// scansione della waiting list
		aux = os->waiting.first;
		while(aux) {
			PCB* pcb = (PCB*)aux;
			aux = aux->next;
			ProcessEvent* e = (ProcessEvent*) pcb->events.first;
			printf("\twaiting process: (%d)\n", pcb->pid);

			// controlliamo che il pcb abbia l'evento di tipo IO
			assert(e->type==IO);
			
			// è passata un'epoca: decremento della durata rimanente
			e->duration-- ;
			printf("\t\tremaining time: %d\n",e->duration);
			
			// se l'IO BURST non è terminato, si passa al prossimo processo
			// se l'IO BURST è terminato,
			if (e->duration == 0) {
				printf("\t\tend IO BURST for process (%d)\n", pcb->pid);
				
				// eliminazione dell'evento dalla coda degli eventi del pcb
				List_popFront(&pcb->events);
				free(e);

				// eliminazione del pcb dalla waiting list
				List_detach(&os->waiting, (ListItem*)pcb);
				
				// eventi finiti: kill process
				if (! pcb->events.first) {
					printf("\t\tend process (%d)\n", pcb->pid);
					free(pcb);
				}
				// ci sono ancora eventi di tipo CPU o IO
				else {
					e = (ProcessEvent*) pcb->events.first;

					// trasferimento del pcb in ready o waiting list
					switch (e->type){
					case CPU:
						printf("\t\t(%d) move to ready\n", pcb->pid);
						List_pushBack(&os->ready, (ListItem*) pcb);
						break;
					case IO:
						printf("\t\t(%d) move to waiting\n", pcb->pid);
						List_pushBack(&os->waiting, (ListItem*) pcb);
						break;
					}
				}
			}
		}

		// stampa processi in running
		aux = os->running.first;
		printf("\t%d processes are running:", runningProcess);
		if(!runningProcess) printf("\t----");
		while(aux) {
			PCB* pcb = (PCB*)aux;
			printf("\t(%d)", pcb->pid);
		}
		printf("\n");

		// scansione dei processi in running
		aux = os->running.first;
		while(aux) {
			PCB* pcb = (PCB*)aux;
			aux = aux->next;
			ProcessEvent* e = (ProcessEvent*) pcb->events.first;
			printf("\trunning pid: (%d)\n", pcb->pid);
			
			// controlliamo che il pcb abbia l'evento di tipo CPU
			assert(e->type==CPU);

			// è passata un'epoca: decremento della durata rimanente
			e->duration--;
			printf("\t\tremaining time:%d\n",e->duration);

			// se il CPU BURST non è terminato, si passa al prossimo processo
			// se il CPU BURST è terminato,
			if (e->duration == 0) {
				printf("\t\tend CPU BURST for process (%d)\n", pcb->pid);

				// eliminazione dell'evento dalla coda degli eventi del pcb
				List_popFront(&pcb->events);
				free(e);

				// eliminazione del pcb dalla running list
				List_detach(&os->running, (ListItem*)pcb);

				// eventi finiti: kill process
				if (! pcb->events.first) {
					printf("\t\tend process (%d)\n", pcb->pid);
					free(pcb);
				}
				// ci sono ancora eventi di tipo CPU o IO
				else {
					e = (ProcessEvent*) pcb->events.first;

					// trasferimento del pcb in ready o waiting list
					switch (e->type){
					case CPU:
						printf("\t\t(%d) move to ready\n", pcb->pid);
						List_pushBack(&os->ready, (ListItem*) pcb);
						break;
					case IO:
						printf("\t\t(%d) move to waiting\n", pcb->pid);
						List_pushBack(&os->waiting, (ListItem*) pcb);
						break;
					}
				}
				// os->running = 0;
			}
		}


	}

	++os->timer;

}

void OS_destroy(OS* os) {
}
