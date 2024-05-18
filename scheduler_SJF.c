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

/* La funzione schedulerSJF() viene chiamata dall'OS per individuare il PCB in coda ready con il minor CPU 
	burst previsto. Il PCB viene dunque inserito nella coda dei processi in running.
*/
void schedulerSJF(OS* os, void* args_){
	// scheduler_args* args = (scheduler_args*)args_;

	// Si prende il primo processo dalla lista ready
	ListItem* item = os->ready.first;

	// Se la ready list è vuota, l'algoritmo termina
	if (!item) {
		#ifdef _DEBUG_SCHEDULER
			printf("SCHEDULER: DO NOTHING (no pcb in ready)\n");
		#endif
		return;
	}

	/* Se la lista ready non è vuota, viene individuato il processo il cui prossimo evento CPU burst ha minor
		durata attraverso la funzione shortestJobPCB(). */
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
};


/* La funzione SJF_calculatePrediction() viene chiamata dall'OS per gestire i casi in cui il CPU burst termina
	o supera la durata del quanto.
*/
void SJF_calculatePrediction(PCB* pcb, void* args_) {
	scheduler_args* args = (scheduler_args*)args_;

	// Controlli sul prossimo evento
	assert(pcb->events.first);
	ProcessEvent* e = (ProcessEvent*)pcb->events.first;
	assert(e->type==CPU);

	#ifdef _DEBUG_SCHEDULER
		printf("\nduration: %d\tquantum: %.2f\tnextprediction: %.2f\n", e->duration, e->quantum, e->next_prediction);
	#endif

	/* Caso di CPU burst finito:
		Quando un evento di tipo CPU burst termina, viene calcolata la predizione per il prossimo CPU burst e 
		la si pone come valore del quanto per il prossimo CPU burst. */
	if(e->timer == e->duration) {

		// Caso del primo CPU burst di un processo
		if(e->quantum==-1) {
			e->quantum = e->timer;
			e->next_prediction = pcb->timer;
		}

		// Caso dei successivi CPU burst di un processo o ripresa del primo CPU burst
		else {
			// Caso di ripresa del primo CPU burst, interrotto precedentemente dal quanto
			if(pcb->last_prediction == -1) e->next_prediction = pcb->timer;

			// Caso di CPU burst successivi al primo
			else {
				/* Calcolo previsione del prossimo burst:
					Viene applicato il filtro passa basso tra il tempo totale misurato per la terminazione
					completa del CPU burst e il tempo previsto alla fine dello scorso CPU burst. */
				double lpFilter = pcb->timer * args->decay + pcb->last_prediction * (1-args->decay);
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

				// Il quanto del CPU burst successivo è pari alla previsione fatta nell'epoca corrente
				next_e->quantum = e->next_prediction;
				break;
			}
		}

		/* La previsione fatta nell'epoca corrente viene salvata in pcb->last_prediction al fine di
			poter essere riutilizzata in futuro per il calcolo delle previsioni.*/
		pcb->last_prediction = e->next_prediction;

		// Lo scheduler avvisa l'OS di resettare il pcb->timer
		pcb->resetTimer = 1;
	}
	
	/* Caso di CPU burst interrotto dal quanto (o max_quantum):
		Se un evento è in running da più tempo del valore del quanto (o max_quantum) viene interrotto 
		dallo scheduler. Verrà creato un nuovo evento di tipo CPU burst avente come durata la durata rimanente
		del burst interrotto. */
	else {
		pcb->stoppedByQuantum = 1;
			
		// Creazione di un nuovo evento con la durata del CPU burst interrotto
		ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
		qe->list.prev = 0;
		qe->list.next = 0;
		qe->type = CPU;
		qe->timer = 0;

		// Caso del primo CPU burst di un processo
		if(e->quantum==-1) e->quantum = e->timer;

		/* Operazione sconosciuta allo scheduler: la sua unica funzionalità
			è di rendere noto il momento in cui termina un CPU burst. 	
		####################################### */
		qe->duration = e->duration - e->timer;
		/* #################################### */
		
		/* Calcolo previsione del prossimo burst:
			Viene applicato il filtro passa basso tra il tempo totale misurato per la terminazione
			completa del CPU burst e il quanto di tempo corrente.
			Se quest'ultimo equivale a -1, caso del primo CPU burst di un processo, abbiamo provveduto
			a settarlo ad un valore pari a e->timer.

		Questa operazione ha come scopo quello di minimizzare le interruzioni del CPU burst quando vi 
			sono dei notevoli aumenti da un burst all'altro. */
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

	/* A parità di quanto, viene selezionato quello corrispondente al PCB più "a sinistra" della coda, cioè
		quello non utilizzato da più tempo. 
	Se ci sono eventi con quanto pari a -1, corrispondenti a PCB appena creati, viene scelto 
		il PCB già esistente. */
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