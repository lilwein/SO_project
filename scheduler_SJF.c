#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "os.h"
#include "scheduler.h"
#include "aux.h"

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
	scheduler_args* args = (scheduler_args*)args_;

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

	#ifdef _DEBUG_SCHEDULER
		printf("\nduration: %d\tquantum: %.2f\tnextprediction: %.2f\n", e->duration, e->quantum, e->next_prediction);
	#endif

	// /* Se la durata dell'evento supera quella del quantum prediction, si deve dividere l'evento in due eventi.
	// 	Il primo evento, qe, viene creato e differisce da quello originale poichè la sua durata equivale al
	// 	quantum prediction. Il secondo evento, che segue l'evento qe creato, è l'evento originale stesso ma con 
	// 	la durata rimasta.
	// */
	// if ( e->duration > e->quantum ) {

	// 	// Il pcb deve essere fermato dall'OS
	// 	pcb->stoppedByQuantum = 1;
		
	// 	// Nuovo evento nato dalla scissione
	// 	ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
	// 	qe->list.prev = 0;
	// 	qe->list.next = 0;
	// 	qe->type = CPU;
	// 	qe->timer = 0;

	// 	qe->duration = e->quantum;
	// 	qe->quantum = e->quantum;

	// 	/* Timer che conta a partire dalla scissione dell'evento. 
	// 	pcb->timer avrà lo stesso valore di timerFromSplit nel momento in cui l'OS runnerà l'ultimo
	// 	evento CPU burst. Questo passaggio serve a sincronizzare lo shceduler con pcb->timer dato che viene
	// 	eseguito prima che l'OS elabori l'evento. */
	// 	int timerFromSplit = pcb->timer + qe->duration;
		
	// 	double lpFilter = timerFromSplit * args->decay + qe->quantum * (1-args->decay);
	// 	qe->next_prediction = round(lpFilter);
		
	// 	e->duration = e->duration - e->quantum;
	// 	e->quantum = qe->next_prediction;
	// 	e->timer = 0;

	// 	// qe viene inserito in cima alla lista degli eventi
	// 	List_pushFront(&pcb->events, (ListItem*)qe);
		
	// 	#ifdef _DEBUG_SCHEDULER
	// 		printf("\ntimerFromSplit: %d\tlpFilter: %f\tround: %f\tqe->np: %d\n", timerFromSplit, lpFilter, round(lpFilter), qe->next_prediction);
	// 	#endif
	// }

	// // Se la durata dell'evento è minore del quanto, si avvisa l'OS di resettare il pcb->timer
	// if(e->duration <= e->quantum) pcb->resetTimer = 1;
};


void SJF_calculatePrediction(PCB* pcb, void* args_) {
	scheduler_args* args = (scheduler_args*)args_;

	// Controlli sul prossimo evento
	assert(pcb->events.first);
	ProcessEvent* e = (ProcessEvent*)pcb->events.first;
	assert(e->type==CPU);

	// Burst finito
	if(e->timer == e->duration) {

		// Caso di una run del primo CPU burst di un processo
		if(e->quantum==-1) {
			e->quantum = e->timer;
			e->next_prediction = pcb->timer;
		}
		else {
			// Calcolo previsione del prossimo burst
			if(pcb->last_cpu_burst == -1) e->next_prediction = pcb->timer;
			else {
				double lpFilter = pcb->timer * args->decay + pcb->last_cpu_burst * (1-args->decay);
				e->next_prediction = lpFilter;
			}
		}

		// Selezione dell'evento CPU successivo e inizializzazione del quanto
		ListItem* aux = pcb->events.first->next;
		while(aux){
			ProcessEvent* next_e = (ProcessEvent*) aux;
			if(next_e->type==IO) aux = aux->next;
			else{
				assert(next_e->type==CPU);
				next_e->quantum = e->next_prediction;
				break;
			}
		}

		// 
		pcb->last_cpu_burst = e->next_prediction;

		// Lo scheduler avvisa l'OS di resettare il pcb->timer
		pcb->resetTimer = 1;
	}

	// Burst interrotto dal quanto o dal max_quantum
	else {
		pcb->stoppedByQuantum = 1;
			
		// Nuovo evento nato dalla scissione
		ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
		qe->list.prev = 0;
		qe->list.next = 0;
		qe->type = CPU;
		qe->timer = 0;

		// Caso di un processo alla prima run
		if(e->quantum==-1) e->quantum = e->timer;

		/* Operazione sconosciuta allo scheduler: la sua unica funzionalità
			è di rendere noto il momento in cui termina un CPU burst. */
		qe->duration = e->duration - e->timer;
		
		// Calcolo previsione del prossimo burst
		double lpFilter = pcb->timer * args->decay + e->quantum * (1-args->decay);
		e->next_prediction = lpFilter;

		// Inizializzazione del quanto per l'evento con il burst rimanente
		qe->quantum = e->next_prediction;

		// Eliminazione dell'evento originale dalla lista degli eventi
		List_popFront(&pcb->events);

		// qe viene inserito in cima alla lista degli eventi
		List_pushFront(&pcb->events, (ListItem*)qe);
	}

}


/* shortestJobPCB() è una funzione ricorsiva che scandisce una lista di pcb e restituisce un puntatore al pcb
	il cui prossimo evento ha durata minore (ma maggiore di 0).
	Vengono accettati solo i processi che non siano già stati utilizzati dallo scheduler (sia in running che
	in waiting) nell'epoca corrente.
	Se tutti i processi hanno il prossimo evento con quantum = -1 (come succede all'inizio della simulazione),
	verrà selezionato il primo processo in coda.
	A parità di quantum, viene selezionato il pcb più "a sinistra" della lista ready, cioè quello in attesa da più tempo.
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
	
	#ifdef _DEBUG_SCHEDULER
		printf("\n\ne->quantum: %.2f\tnext_e->quantum: %.2f\n", e->quantum,next_e->quantum);
	#endif

	if( e->quantum <= next_e->quantum) {
		if( e->quantum > 0 ) return pcb;
		if( next_e->quantum > 0 ) return next_pcb;
		return pcb;
	}
	else {
		if( next_e->quantum > 0 ) return next_pcb;
		if( e->quantum > 0 ) return pcb;
		return pcb;
	}
};