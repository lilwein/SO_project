#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "process.h"

#define LINE_LENGTH 1024

#define DECAY_COEFF 0.5

int Process_load(Process* p, const char* filename) {
	
	FILE* f = fopen(filename, "r");
	if (!f) return -1;
	
	char *buffer = NULL;
	size_t line_length = 0;
	p->pid = -1;
	p->arrival_time = -1;
	
	List_init(&p->events);
	p->list.prev = 0;
	p->list.next = 0;
	
	int num_events = 0;

	while (getline(&buffer, &line_length, f) > 0 ){
		// got line in buf
		int pid = -1;
		int arrival_time = -1;
		int num_tokens = 0;
		int duration = -1;

		num_tokens = sscanf(buffer, "PROCESS %d %d", &pid, &arrival_time);
		if (num_tokens==2 && p->pid<0){
			p->pid = pid;
			p->arrival_time = arrival_time;
			continue;
		}

		num_tokens = sscanf(buffer, "CPU_BURST %d", &duration);
		if (num_tokens==1){
			// we create a new event of type cpu burst
			ProcessEvent* e = (ProcessEvent*) malloc(sizeof(ProcessEvent));
			e->list.prev = 0;
			e->list.next = 0;
			e->type = CPU;
			e->duration = duration;
			e->prediction = -1;
			e->next_prediction = -1;
			List_pushBack(&p->events, (ListItem*)e);
			++num_events;
			continue;
		}
		num_tokens=sscanf(buffer, "IO_BURST %d", &duration);
		if (num_tokens==1){
			// we create a new event of type cpu burst
			ProcessEvent* e = (ProcessEvent*) malloc(sizeof(ProcessEvent));
			e->list.prev = 0;
			e->list.next = 0;
			e->type = IO;
			e->duration = duration;
			e->prediction = -1;
			e->next_prediction = -1;
			List_pushBack(&p->events, (ListItem*)e);
			++num_events;
			continue;
		}
	}

	if (buffer)
		free(buffer);
	fclose(f);

	return num_events;
}


void Process_CalculatePrediction(Process* p){
	
	ListItem* aux = p->events.first;
	int quantum_pred;
	
	char first_event = 1;

	while(aux){
		ProcessEvent* e = (ProcessEvent*) aux;
		assert(e->type == CPU);
		assert(e->prediction == -1);
		
		if(first_event) e->prediction = e->duration;
		else e->prediction = quantum_pred;

		double qp = e->duration * DECAY_COEFF + e->prediction * (1-DECAY_COEFF);
		quantum_pred = round(qp);
		e->next_prediction = quantum_pred;

		first_event = 0;
		aux = aux->next;
		if(aux==NULL) break;
		aux = aux->next;
	}
}


int Process_save(const Process* p, const char* filename){
	
	FILE* f = fopen(filename, "w");
	if (!f) return -1;

	fprintf(f, "PROCESS %d %d\n", p->pid, p->arrival_time);
	ListItem* aux = p->events.first;
	int num_events;

	while(aux) {
		ProcessEvent* e = (ProcessEvent*) aux;

		switch(e->type){
		case CPU:
			fprintf(f, "CPU_BURST %d\n", e->duration);
			++ num_events;
			break;
		case IO:
			fprintf(f, "IO_BURST %d\n", e->duration);
			++ num_events;
			break;
		default:;
		}

		aux = aux->next;
	}

	fclose(f);
	return num_events;
}
	
