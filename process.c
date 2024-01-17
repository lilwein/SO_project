#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "process.h"

#define LINE_LENGTH 1024

int Process_load_file(Process* p, const char* filename) {
	
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
			e->quantum = -1;
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
			e->quantum = -1;
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

void Process_init_inline(Process* p, int pid, int arrival){
	p->pid = pid;
	p->arrival_time = arrival;
	
	List_init(&p->events);
	p->list.prev = 0;
	p->list.next = 0;
};

ProcessEvent* Process_load_inline(Process* p, int cpu_burst, int io_burst) {

	ProcessEvent* e_CPU = (ProcessEvent*) malloc(sizeof(ProcessEvent));
	e_CPU->list.prev = 0;
	e_CPU->list.next = 0;
	e_CPU->type = CPU;
	e_CPU->duration = cpu_burst;
	e_CPU->quantum = -1;
	e_CPU->next_prediction = -1;
	List_pushBack(&p->events, (ListItem*)e_CPU);

	ProcessEvent* e_IO = (ProcessEvent*) malloc(sizeof(ProcessEvent));
	e_IO->list.prev = 0;
	e_IO->list.next = 0;
	e_IO->type = IO;
	e_IO->duration = io_burst;
	e_IO->quantum = -1;
	e_IO->next_prediction = -1;
	List_pushBack(&p->events, (ListItem*)e_IO);

	return e_CPU;
};

void Process_CalculatePrediction(Process* p, double decay, ProcessEvent* start){
	
	int quantum_pred;
	char first_event = 1;

	ListItem* aux = p->events.first;
	
	if(start && List_find(&p->events, (ListItem*)start) ){
		while(aux && (ProcessEvent*)aux != start ){
			ProcessEvent* e = (ProcessEvent*) aux;
			if(e->type==CPU) quantum_pred = e->next_prediction;
			aux = aux->next;
			first_event = 0;
		}
		// if(aux->prev){
		// 	quantum_pred = ((ProcessEvent*) aux->prev)->next_prediction;
		// 	first_event = 0;
		// }
	}

	while(aux){
		ProcessEvent* e = (ProcessEvent*) aux;
		assert(e->type == CPU);
		assert(e->quantum == -1);
		assert(e->next_prediction == -1);
		
		if(first_event) e->quantum = e->duration;
		else e->quantum = quantum_pred;

		double qp = e->duration * decay + e->quantum * (1-decay);
		quantum_pred = round(qp);
		e->next_prediction = quantum_pred;

		first_event = 0;
		aux = aux->next;
		if(aux==NULL) break;
		e = (ProcessEvent*) aux;
		assert(e->type == IO);
		aux = aux->next;
	}
}


int Process_save_file(const Process* p, const char* filename){
	
	char* path = "./output/";
	char* extension = ".txt";
	char* name = (char*) malloc(strlen(path)+strlen(extension)+5);
	strcat(name, path);
	strcat(name, filename);
	strcat(name, extension);

	FILE* f = fopen(name, "w");
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
	
