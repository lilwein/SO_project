#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "os.h"
#include "scheduler.h"

/*
L'algoritmo Shortest Job First per la scelta del processo da eseguire consiste nello scegliere
	il processo con il minor CPU burst, cioè quello che terrà meno occupata la CPU.
	Il limite di questo algoritmo sta nell'impossibilità di conoscere il futuro comportamento di
	un processo e quindi di stabilirne i tempi di CPU e IO burst.
E' possibile rimediare a questo problema cercando di prevedere il futuro comportamento del processo
	che, tendenzialmente, è	ciclico: le durate dei CPU burst potranno variare ma si terranno mediamente
	su un certo valore, così come per gli IO burst.
Una possibile approssimazione si ottiene applicando un filtro passa basso in grado di attenuare
	l'effetto di burst eccezionali. Il tempo stimato del prossimo burst sarà dunque una media tra il tempo
	effettivamente misurato del burst corrente ed il tempo precedentemente stimato per il burst corrente:
										B(t+1) = a * b(t) + (1-a) * B(t)
	Il coefficiente 'a' serve ad attribuire nel calcolo della media un peso al tempo misurato b(t) e al
	tempo precedentemente stimato B(t).

Nell'algoritmo che andremo ad implementare, applicheremo all'idea dello Shortest Job First il concetto di
	preemption: lo scheduler può togliere forzatamente la CPU ad un processo se questo la sta usando da più
	di un periodo di tempo chiamato cpu quantum.
Per integrare il concetto di preemption con lo SJB, andremo a predire il prossimo CPU burst attraverso il
	filtro passa basso, e considereremo questo valore come il quanto di tempo dopo il quale verrà tolta la
	cpu al processo corrente.
Nel caso base, cioè quando un processo arriva, useremo la durata reale dell'evento come quantum prediction.
*/

void schedulerSJF(OS* os, void* args_){
	//scheduler_args* args = (scheduler_args*)args_;

	// Si prende il primo processo dalla lista ready
	ListItem* item = os->ready.first;

	// Se la ready list è vuota, l'algoritmo termina
	if (!item) {
		#ifdef _DEBUG_SCHEDULER
			printf("SCHEDULER: DO NOTHING (no pcb in ready)\n");
		#endif
		return;
	}

	/* Se la lista ready non è vuota, viene individuato il processo il cui prossimo evento CPU burst ha mino
	durata attraverso la funzione shortestJobPCB()
	*/
	PCB* pcb = (PCB*) malloc(sizeof(PCB));
	pcb = shortestJobPCB(item);

	// Se tutti i processi nella ready list sono già stati utilizzati in quest'epoca, l'algoritmo termina
	if (!pcb) {
		#ifdef _DEBUG_SCHEDULER
			printf("SCHEDULER: DO NOTHING (no pcb can run this time)\n");
		#endif
		return;
	}
	
	#ifdef _DEBUG_SCHEDULER
		printf("SCHEDULER: PUT ready.first pid (%d) in running\n", ((PCB*) item)->pid);
	#endif

	// Il pcb con l'evento più breve viene tolto dalla ready list e viene aggiunto alla running list
	List_detach( &(os->ready), (ListItem*) pcb);
	List_pushBack( &(os->running), (ListItem*) pcb);
	
	// Controlli sul prossimo evento
	assert(pcb->events.first);
	ProcessEvent* e = (ProcessEvent*)pcb->events.first;
	assert(e->type==CPU);

	// printf("\nduration: %d\tquantum: %d\tnextprediction: %d\n", e->duration, e->quantum, e->next_prediction);

	/* Se la durata dell'evento supera quella del quantum prediction, si deve dividere l'evento in due eventi.
		Il primo evento, qe, viene creato e differisce da quello originale poichè la sua durata equivale al
		quantum prediction. Il secondo evento, che segue l'evento qe creato, è l'evento originale stesso ma con 
		la durata rimasta.
	*/
	if ( e->duration > e->quantum ) { 
		// printf("\nprocess (%d) hasn't done yet, CPU Burst event divided\n", ((PCB*)os->running.first)->pid );
		ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
		qe->list.prev = 0;
		qe->list.next = 0;
		qe->type = CPU;

		qe->duration = e->quantum;
		qe->quantum = e->next_prediction;
		qe->next_prediction = e->next_prediction;

		e->duration = e->duration - e->quantum;
		e->quantum = e->next_prediction;

		List_pushFront(&pcb->events, (ListItem*)qe);
	}
};


/* shortestJobPCB() è una funzione ricorsiva che scandisce una lista di pcb e restituisce un puntatore al pcb
	il cui prossimo evento ha durata minore.
	Vengono accettati solo i processi che non siano già stati utilizzati dallo scheduler (sia in running che
	in waiting) nell'epoca corrente.
	Restituisce NULL se nessun processo ha soddisfatto la precedente condizione.
*/
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
	if( e->quantum <= next_e->quantum ) return pcb;
	else return next_pcb;
};