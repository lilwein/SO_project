#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "os.h"

OS os;

/*
L'algoritmo Shortest Job First per la scelta del processo da eseguire consiste nello scegliere il processo con
	il minor CPU burst, cioè quello che terrà meno occupata la CPU.
	Il limite di questo algoritmo sta nell'impossibilità di conoscere il futuro comportamento di un processo e quindi
	di stabilirne i tempi di CPU e IO burst.
E' possibile rimediare a questo problema cercando di prevedere il futuro comportamento del processo che è tendenzialmente
	ciclico: le durate dei CPU burst potranno variare ma si terranno mediamente su un certo valore, così come per gli IO 
	burst.
Una possibile approssimazione si ottiene applicando un filtro passa basso in grado di attenuare l'effetto di burst 
	eccezionali. Il tempo stimato del prossimo burst sarà dunque una media tra il tempo effettivamente misurato del burst
	corrente ed il tempo precedentemente stimato per il burst corrente:
										B(t+1) = a * b(t) + (1-a) * B(t)
	Il coefficiente a serve ad attribuire nel calcolo della media il peso del tempo misurato b(t) e quello del tempo 
	precedentemente stimato B(t).

Nell'algoritmo che andremo ad implementare, applicheremo all'idea dello Shortest Job First il concetto di preemption:
	lo scheduler può togliere forzatamente la CPU ad un processo se questo la sta usando da più di un periodo di tempo 
	chiamato cpu quantum.
Per integrare il concetto di preemption con lo SJB, andremo a predire il prossimo CPU burst attraverso il filtro passa
	basso, e considereremo questo valore come il quanto di tempo dopo il quale verrà tolta la cpu al processo corrente.
*/




void schedulerSJF(OS* os, void* args_){
	//scheduler_args* args = (scheduler_args*)args_;

	// Si prende il primo processo dalla lista ready
	ListItem* item = os->ready.first;

	// Se la lista ready è vuota, l'algoritmo termina
	if (!item) {
		#ifdef _DEBUG_SCHEDULER
			printf("SCHEDULER: DO NOTHING (no pcb in ready)\n");
		#endif
		return;
	}

	/*Se la lista ready non è vuota, viene individuato il processo il cui evento CPU burst imminente ha minor durata
	attraverso la funzione shortestJobPCB()
	*/
	PCB* pcb = (PCB*) malloc(sizeof(PCB));
	pcb = shortestJobPCB(item);

	if (!pcb) {
		#ifdef _DEBUG_SCHEDULER
			printf("SCHEDULER: DO NOTHING (no pcb can run this time)\n");
		#endif
		return;
	}
	
	#ifdef _DEBUG_SCHEDULER
		printf("SCHEDULER: PUT ready.first pid (%d) in running\n", ((PCB*) item)->pid);
	#endif

	List_detach( &(os->ready), (ListItem*) pcb);

	//  *** aggiunta pcb a running
	List_pushBack( &(os->running), (ListItem*) pcb);
	
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
	if(!next_pcb){
		if(pcb->usedThisTime) return NULL;
		else return pcb;
	}

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
