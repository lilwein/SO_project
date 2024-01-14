#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>

#include "aux.h"
#include "os.h"

#define MAX_PROCESSES 1024

int stampa = 0;

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

	new_pcb->usedThisTime = 0;

	// controlliamo che il processo p abbia eventi
	assert( new_pcb->events.first && "Error on creation: process without events");

	// inserimento del pcb in ready o waiting list
	ProcessEvent* e = (ProcessEvent*)new_pcb->events.first;
	
	switch(e->type){
	case CPU:
		List_pushBack(&os->ready, (ListItem*) new_pcb);
		// List_pushBack(&os->runnable, (ListItem*) new_pcb);
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
	// printf("\e[5mMd.Mehedi hasan\e[m");
	// printf("\x1b[2;37;41mWorld");
	// printf("\e[1;31maaaaaaaaaa\e[0m");

	printEscape("7;1;8"); printf("\n*************************"); printEscape("28");
	printf("TIME: %08d", os->timer);
	printEscape("8"); printf("*************************\n"); printEscape("0");


	// selezione dei processi arrivati al tempo os->timer
	// trasformazione dei processi pronti in pcb 
	// inserimento dei pcb in ready o waiting list
	ListItem* aux = os->processes.first;
	int create = 0;
	while (aux){
		Process* proc = (Process*) aux;
		Process* new_process = 0;
		if (proc->arrival_time == os->timer){
			new_process = proc;
		}
		aux = aux->next;
		if (new_process) {
			if(!create){
				printf("----------------------------------------------------------------\n");
				printEscape("1;3"); printf("\tCreate process with pid:"); printEscape("0");
			}
			printf(" "); printEscape("1;3;44"); printf("(%d)", new_process->pid); printEscape("0");
			new_process = (Process*) List_detach(&os->processes, (ListItem*)new_process);
			OS_createProcess(os, new_process);
			free(new_process);
			create++;
		}
	}
	if(create) printf("\n");
	
	// numero di core disponibili
	scheduler_args* args = (scheduler_args*) os->schedule_args;
	int core = args->core;

	int runProcessTime = 0;
	int* runPids = (int*) malloc(core*sizeof(int));

	// ripetere per ogni core disponibile
	for(int i=0; i<core; i++){
		
		printf("----------------------------------------------------------------\n");
		printEscape("1;3;7"); printf("\tCORE %d \n", i+1); printEscape("0");

		#ifdef _DEBUG
			printPidListsDebug(os, 1);
		#endif

		// controllo sul numero di processi in running

		assert( os->running.size <= core && "Error: too many process in running" );
		assert( os->running.size >= 0 && "Error: invalid size of running list" );

		// invocare lo scheduler se verificate le seguenti condizioni: 
		// 	1) scheduler è definito;
		//	2) lista processi running non piena.

		// Osservazioni:
		// 	- La capienza massima della lista dei processi in running equivale al numero di core disponibili
		//	- Se la lista dei processi in running è piena, NON viene invocato lo scheduler: la preemption è
		//		gestita dalla durata rimanente degli eventi
		//	- La rimozione di un processo dalla running list è gestita in seguito in questa funzione
		//	- L'aggiunta di un processo alla running list è gestita dallo scheduler 

		if (os->schedule_fn && os->running.size < core){
			(*os->schedule_fn) (os, os->schedule_args);
		}

		if(stampa)printPidLists(os);

		#ifdef _DEBUG
			printPidListsDebug(os, 2);
		#endif

		
		
		

		// scansione della waiting list
		aux = os->waiting.first;
		while( aux && ((PCB*)aux)->usedThisTime == 0 ) {
			PCB* pcb = (PCB*)aux;
			aux = aux->next;

			#ifdef _DEBUG
				printf("in waiting while: pid (%d)\n", pcb->pid);
			#endif

			pcb->usedThisTime = 1;

			#ifdef _DEBUG
				printPidListsDebug(os, 3);
			#endif

			
			ProcessEvent* e = (ProcessEvent*) pcb->events.first;
			printf("\t\twaiting process: ");
			printEscape("1;3;43"); printf("(%d)", pcb->pid); printEscape("0"); printf("\n");

			// controlliamo che il pcb abbia l'evento di tipo IO
			assert(e->type==IO);
			
			// è passata un'epoca: decremento della durata rimanente
			e->duration-- ;
			printf("\t\t\tremaining time: %d\n",e->duration);
			
			// se l'IO BURST non è terminato, si passa al prossimo processo
			// se l'IO BURST è terminato,
			if (e->duration == 0) {
				printf("\t\t\tend IO BURST for process (%d)\n", pcb->pid);
				
				// eliminazione dell'evento dalla coda degli eventi del pcb
				List_popFront(&pcb->events);
				free(e);

				// eliminazione del pcb dalla waiting list
				List_detach(&os->waiting, (ListItem*)pcb);
				
				// eventi finiti: kill process
				if (! pcb->events.first) {
					printf("\t\t\tend process (%d)\n", pcb->pid);
					free(pcb);
				}
				// ci sono ancora eventi di tipo CPU o IO
				else {
					e = (ProcessEvent*) pcb->events.first;

					// trasferimento del pcb in ready o waiting list
					switch (e->type){
					case CPU:
						printf("\t\t\t(%d) move to ready\n", pcb->pid);
						List_pushBack(&os->ready, (ListItem*) pcb);
						break;
					case IO:
						printf("\t\t\t(%d) move to waiting\n", pcb->pid);
						List_pushBack(&os->waiting, (ListItem*) pcb);
						break;
					}
				}
			}
		}



		#ifdef _DEBUG
			printPidListsDebug(os, 4);
		#endif

		// scansione dei processi in running
		aux = os->running.first;

		#ifdef _DEBUG
			// if(aux)printf("OS: runnable (%d)? %s\n", ((PCB*)aux)->pid, ((PCB*)aux)->usedThisTime==0? "si":"no");
		#endif


		while( aux ) {
			PCB* pcb = (PCB*)aux;
			aux = aux->next;

			// ***************
			if(pcb->usedThisTime != 0) continue;

			#ifdef _DEBUG
				printf("in running while: pid (%d)\n", pcb->pid);
			#endif

			pcb->usedThisTime = 1;

			assert(runProcessTime<core);
			runPids[runProcessTime] = pcb->pid;
			runProcessTime++;


			#ifdef _DEBUG
				printPidListsDebug(os, 5);
			#endif
			
			
			ProcessEvent* e = (ProcessEvent*) pcb->events.first;
			printf("\t\trunning process: ");
			printEscape("1;3;48;5;28"); printf("(%d)", pcb->pid); printEscape("0"); printf("\n");
			
			// controlliamo che il pcb abbia l'evento di tipo CPU
			assert(e->type==CPU);

			// è passata un'epoca: decremento della durata rimanente
			e->duration--;
			printf("\t\t\tremaining time: %d\n",e->duration);

			// se il CPU BURST non è terminato, si passa al prossimo processo
			// se il CPU BURST è terminato,
			if (e->duration == 0) {
				printf("\t\t\tend CPU BURST for process (%d)\n", pcb->pid);

				// eliminazione dell'evento dalla coda degli eventi del pcb
				List_popFront(&pcb->events);
				free(e);

				// eliminazione del pcb dalla running list
				List_detach(&os->running, (ListItem*)pcb);

				// eventi finiti: kill process
				if (! pcb->events.first) {
					printf("\t\t\tend process (%d)\n", pcb->pid);
					free(pcb);
				}
				// ci sono ancora eventi di tipo CPU o IO
				else {
					e = (ProcessEvent*) pcb->events.first;

					// trasferimento del pcb in ready o waiting list
					switch (e->type){
					case CPU:
						printf("\t\t\t(%d) move to ready\n", pcb->pid);
						List_pushBack(&os->ready, (ListItem*) pcb);
						break;
					case IO:
						printf("\t\t\t(%d) move to waiting\n", pcb->pid);
						List_pushBack(&os->waiting, (ListItem*) pcb);
						break;
					}
				}
			}
			// ***************************
			break;
		}
		
		if(stampa)printPidLists(os);

		#ifdef _DEBUG
			printPidListsDebug(os, 6);
		#endif
		

	}

	// stampa processi runnati
		printf("----------------------------------------------------------------\n");
		if(!runProcessTime) printf("\tno process has been run");
		else printf("\t%d processes have been run:", runProcessTime);
		for(int k=0; k<runProcessTime; k++){
			printf(" (%d)", runPids[k]);
		}
		printf("\n----------------------------------------------------------------");
		printf("\n");

	setZeroUsed(&os->running);
	setZeroUsed(&os->waiting);
	setZeroUsed(&os->ready);

	++os->timer;

	// sleep(1);

}

void OS_destroy(OS* os) {
}

