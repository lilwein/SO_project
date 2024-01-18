#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "os.h"
#include "aux.h"

#define MAX_PROCESSES 1024

char stampa = 1;

void OS_init(OS* os) {
	List_init(&os->running);
	List_init(&os->ready);
	List_init(&os->waiting);
	List_init(&os->processes);
	List_init(&os->all_processes);
	
	os->timer = 0;
	os->schedule_fn = 0;
}

void OS_createProcess(OS* os, Process* p) {
	// Controlliamo se il processo p è arrivato ora
	assert( p->arrival_time == os->timer && "Error on creation: time mismatch");

	// Controlliamo che nessun processo in running abbia lo stesso pid del processo p
	ListItem * aux = os->running.first;
	while(aux){
		PCB* pcb = (PCB*) aux;
		assert( pcb->pid != p->pid && "Error in running list: pid taken");
		aux = aux->next;
	}
	
	// Controlliamo che nessun processo in ready abbia lo stesso pid del processo p
	aux = os->ready.first;
	while(aux){
		PCB* pcb = (PCB*) aux;
		assert( pcb->pid != p->pid && "Error in ready list: pid taken");
		aux = aux->next;
	}

	// Controlliamo che nessun processo in waiting abbia lo stesso pid del processo p
	aux = os->waiting.first;
	while(aux){
		PCB* pcb = (PCB*)aux;
		assert( pcb->pid != p->pid && "Error in waiting list: pid taken");
		aux = aux->next;
	}

	// Nessun pcb con pid uguale
	PCB* new_pcb = (PCB*) malloc(sizeof(PCB));
	new_pcb->list.next = 0;
	new_pcb->list.prev = 0;
	new_pcb->pid = p->pid;
	new_pcb->events = p->events;
	new_pcb->usedThisTime = 0;
	new_pcb->waitingToRun = 0;

	// Controlliamo che il processo p abbia eventi
	assert( new_pcb->events.first && "Error on creation: process without events");

	// Inserimento del pcb in ready o waiting list
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
	}
}




void OS_simStep(OS* os, int* timer){
	*timer = os->timer;

	printEscape("7;1;8"); printf("\n*************************"); printEscape("28");
	printf("TIME: %08d", os->timer);
	printEscape("8"); printf("*************************\n"); printEscape("0");

	// Selezione dei processi arrivati al tempo os->timer
	// Trasformazione dei processi pronti in pcb 
	// Inserimento dei pcb in ready o waiting list
	ListItem* aux = os->processes.first;
	char create = 0;
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
	} if(create) printf("\n");
	
	// Numero di core disponibili
	scheduler_args* args = (scheduler_args*) os->schedule_args;
	int core = args->core;

	int runProcessTime = 0;
	int* runPids = (int*) malloc(core*sizeof(int));

	// Stampa liste
	// if(stampa)printPidLists(os);

	// RIPETERE PER OGNI CORE
	for(int i=0; i<core; i++){
		printf("----------------------------------------------------------------\n");
		printEscape("1;3;7"); printf("\tCORE %d \n", i+1); printEscape("0");
		#ifdef _DEBUG
			printPidListsDebug(os, 1);
		#endif

		// Controllo sul numero di processi in running
		assert( os->running.size <= core && "Error: too many process in running" );
		assert( os->running.size >= 0 && "Error: invalid size of running list" );


		/* Invocare lo scheduler se verificate le seguenti condizioni: 
			1) scheduler è definito;
			2) lista processi running non piena. */

		/* Osservazioni:
			- La capienza massima della lista dei processi in running equivale al numero di core disponibili;
			- Se la lista dei processi in running è piena, NON viene invocato lo scheduler: la preemption è
				gestita dalla durata rimanente degli eventi;
			- La rimozione di un processo dalla running list è gestita in seguito in questa funzione;
			- L'aggiunta di un processo alla running list è gestita dallo scheduler. */

		if (os->schedule_fn && os->running.size < core){
			(*os->schedule_fn) (os, os->schedule_args);
		}

		// Stampa liste
		// if(stampa)printPidLists(os);

		#ifdef _DEBUG
			printPidListsDebug(os, 2);
		#endif


		// Scansione della WAITING list
		aux = os->waiting.first;

		/* Affinchè si possa consumare un tempo dal processo in waiting, ci si deve prima assicurare che
			il processo non sia stato oggetto di eleborazione in un altro core dell'epoca corrente.
		Nel momento in cui si consuma un'epoca di un processo (che sia in waiting o in running), si pone 
			il campo usedThisTime del PCB ad 1.
		Nella successiva istruzione, si procede a lavore sul processo in waiting solo se il suo campo
			usedThisTime è rimasto 0.
		Alla fine del ciclo for sui core, verranno ripristinati i valori di usedThisTime a 0 per tutti 
			i processi.
		*/
		while( aux && ((PCB*)aux)->usedThisTime == 0 ) {
			PCB* pcb = (PCB*)aux;
			aux = aux->next;

			/* Si pone il campo usedThisTime del processo corrente ad 1 cosicché non verrà preso in
				considerazione dagli altri core. */
			pcb->usedThisTime = 1;

			#ifdef _DEBUG
				printf("in waiting while: pid (%d)\n", pcb->pid);
				printPidListsDebug(os, 3);
			#endif

			printf("\t\twaiting process: ");
			printEscape("1;3;43"); printf("(%d)", pcb->pid); printEscape("0"); printf("\n");
			
			ProcessEvent* e = (ProcessEvent*) pcb->events.first;

			// Controlliamo che il pcb abbia l'evento di tipo IO
			assert(e->type==IO);
			
			// E' passata un'epoca: decremento della durata rimanente
			// Process_save_file(pcb, "1");
			e->duration -- ;
			printf("\t\t\tremaining time: %d\n",e->duration);

			#ifdef _DEBUG
				printf("\nduration: %d\tquantum: %d\tnextprediction: %d\n", e->duration, e->quantum, e->next_prediction);
			#endif
			
			// Se l'IO BURST non è terminato, si passa al prossimo processo
			// Se l'IO BURST è terminato,
			if (e->duration == 0) {
				printf("\t\t\tend IO BURST for process (%d)\n", pcb->pid);
				
				// Eliminazione dell'evento dalla coda degli eventi del pcb
				List_popFront(&pcb->events);
				free(e);

				// Eliminazione del pcb dalla waiting list
				List_detach(&os->waiting, (ListItem*)pcb);
				
				// Eventi finiti: kill process
				if (! pcb->events.first) {
					printf("\t\t\t"); printEscape("1;41"); printf("end process "); printEscape("3");
					printf("(%d)", pcb->pid); printEscape("0"); printf("\n");

					// Creazione copia del pcb che verrà inserita in all_processes
					Short_PCB* pcb_copy = (Short_PCB*) malloc(sizeof(Short_PCB));
					pcb_copy->list.next = 0;
					pcb_copy->list.prev = 0;
					pcb_copy->pid = pcb->pid;
					pcb_copy->usedThisTime = pcb->usedThisTime;
					pcb_copy->waitingToRun = pcb->waitingToRun;
					List_pushBack(&os->all_processes, (ListItem*)pcb_copy);

					free(pcb);
				}
				// Ci sono ancora eventi di tipo CPU o IO
				else {
					e = (ProcessEvent*) pcb->events.first;

					// Trasferimento del pcb in ready o waiting list
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

		// scansione dei processi in RUNNING
		aux = os->running.first;

		while(aux) {
			PCB* pcb = (PCB*)aux;
			aux = aux->next;

			/* Se il processo in causa è stato già utilizzato nell'epoca corrente, si procede ad
				analizzare gli altri eventuali processi in running. */
			if(pcb->usedThisTime) continue;

			#ifdef _DEBUG
				printf("in running while: pid (%d)\n", pcb->pid);
			#endif

			// Incremento CPU utilization per il core corrente
			os->CPUs_utilization[i] ++;

			/* Si pone il campo usedThisTime del processo corrente ad 1 cosicché non verrà preso in
				considerazione dagli altri core. */
			pcb->usedThisTime = 1;

			assert(runProcessTime<core);
			runPids[runProcessTime] = pcb->pid;
			runProcessTime++;

			#ifdef _DEBUG
				printPidListsDebug(os, 5);
			#endif
			
			printf("\t\trunning process: ");
			printEscape("1;3;48;5;28"); printf("(%d)", pcb->pid); printEscape("0"); printf("\n");

			ProcessEvent* e = (ProcessEvent*) pcb->events.first;
			
			// Controlliamo che il pcb abbia l'evento di tipo CPU
			assert(e->type==CPU);

			// E' passata un'epoca: decremento della durata rimanente
			// Process_save_file(pcb, "1");
			e->duration --;
			printf("\t\t\tremaining time: %d\n",e->duration);

			#ifdef _DEBUG
				printf("\nduration: %d\tquantum: %d\tnextprediction: %d\n", e->duration, e->quantum, e->next_prediction);
			#endif

			// Se il CPU BURST non è terminato, si passa al prossimo processo
			// Se il CPU BURST è terminato,
			if (e->duration == 0) {
				printf("\t\t\tend CPU BURST for process (%d)\n", pcb->pid);

				// Eliminazione dell'evento dalla coda degli eventi del pcb
				List_popFront(&pcb->events);
				free(e);

				// Eliminazione del pcb dalla running list
				List_detach(&os->running, (ListItem*)pcb);

				// Eventi finiti: kill process
				if (! pcb->events.first) {
					printf("\t\t\t"); printEscape("1;41"); printf("end process "); printEscape("3");
					printf("(%d)", pcb->pid); printEscape("0"); printf("\n");

					// Creazione copia del pcb che verrà inserita in all_processes
					Short_PCB* pcb_copy = (Short_PCB*) malloc(sizeof(Short_PCB));
					pcb_copy->list.next = 0;
					pcb_copy->list.prev = 0;
					pcb_copy->pid = pcb->pid;
					pcb_copy->usedThisTime = pcb->usedThisTime;
					pcb_copy->waitingToRun = pcb->waitingToRun;
					List_pushBack(&os->all_processes, (ListItem*)pcb_copy);

					free(pcb);
				}
				// Ci sono ancora eventi di tipo CPU o IO
				else {
					e = (ProcessEvent*) pcb->events.first;

					// Trasferimento del pcb in ready o waiting list
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
			/* A differenza del caso del caso precedente in cui un qualsiasi nuemero di processi possono
				contemporaneamente essere in waiting e consumare un'epoca, nel caso di processi in running,
				SOLO UN processo alla volta può essere runnato da un core. */
			break;
		}

		#ifdef _DEBUG
			printPidListsDebug(os, 6);
		#endif
	} // FINE CICLO CORE

	// Stampa processi runnati
	printf("----------------------------------------------------------------\n");
	if(!runProcessTime) printf("\tno process has been run");
	else printf("\t%d processes have been run:", runProcessTime);
	for(int k=0; k<runProcessTime; k++){
		printf(" (%d)", runPids[k]);
	}
	printf("\n----------------------------------------------------------------\n");

	// Stampa liste
	if(stampa)printPidLists(os);
	printf("----------------------------------------------------------------\n");

	//  ************
	aux = os->ready.first;
	while(aux){
		PCB* pcb = (PCB*) aux;
		if(!pcb->usedThisTime) pcb->waitingToRun ++;
		aux = aux->next;
	}

	// Come anticipato prima, si devono ripristinare i valori di usedThisTime a 0 per tutti i processi.
	setZeroUsed(&os->running);
	setZeroUsed(&os->waiting);
	setZeroUsed(&os->ready);

	// Incremento timer del nostro sistema operativo
	++os->timer;
	*timer = os->timer;

	// sleep(1);
}

int OS_run(OS* os){
	return os->running.first || os->ready.first || os->waiting.first || os->processes.first;
}

void OS_destroy(OS* os) {
}

