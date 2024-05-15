#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "process.h"
#include "aux.h"

#define LINE_LENGTH 1024

// Process_load_file() carica un processo dal file filename
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
			ProcessEvent* e = (ProcessEvent*) malloc(sizeof(ProcessEvent));
			e->list.prev = 0;
			e->list.next = 0;
			e->type = CPU;
			e->duration = duration;
			e->quantum = -1;
			e->next_prediction = -1;
			e->timer = 0;
			List_pushBack(&p->events, (ListItem*)e);
			++num_events;
			continue;
		}
		num_tokens=sscanf(buffer, "IO_BURST %d", &duration);
		if (num_tokens==1){
			ProcessEvent* e = (ProcessEvent*) malloc(sizeof(ProcessEvent));
			e->list.prev = 0;
			e->list.next = 0;
			e->type = IO;
			e->duration = duration;
			e->quantum = -1;
			e->timer = 0;
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
};


// Process_save_file() salva il processo [p] in un file chiamato [p->pid].txt
int Process_save_file(const Process* p){
	
	char pid_str[PROC_MAX_LENGHT]; sprintf(pid_str, "%d", p->pid);
	
	char* path = "./temp/p";
	char* extension = ".txt";
	char* name = (char*) malloc(strlen(path)+strlen(extension)+PROC_MAX_LENGHT);
	strcpy(name, path);
	strcat(name, pid_str);
	strcat(name, extension);

	FILE* f = fopen(name, "w");
	if (!f) handle_error("Error on fopen(): ");
	
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

	free(name);
	fclose(f);
	return num_events;
};


// Event_save_file() salva una coppia di eventi nel file del processo [p]
void Event_save_file(const Process* p, int cpu_burst, int io_burst){
	
	char pid_str[PROC_MAX_LENGHT]; sprintf(pid_str, "%d", p->pid);

	char* path = "./temp/p";
	char* extension = ".txt";
	char* name = (char*) malloc(strlen(path)+strlen(extension)+PROC_MAX_LENGHT);
	strcpy(name, path);
	strcat(name, pid_str);
	strcat(name, extension);

	FILE* f = fopen(name, "a");
	if (!f) handle_error("Error on fopen(): ");

	fprintf(f, "CPU_BURST %d\n", cpu_burst);
	fprintf(f, "IO_BURST %d\n", io_burst);

	free(name);
	fclose(f);
};


// Process_init_inline() inizializza il processo [p] definito inline
void Process_init_inline(Process* p, int pid, int arrival){
	p->pid = pid;
	p->arrival_time = arrival;
	
	List_init(&p->events);
	p->list.prev = 0;
	p->list.next = 0;
};


// Process_load_inline() aggiunge una coppia di eventi al processo [p] definito inline
ProcessEvent* Process_load_inline(Process* p, int cpu_burst, int io_burst) {

	ProcessEvent* e_CPU = (ProcessEvent*) malloc(sizeof(ProcessEvent));
	e_CPU->list.prev = 0;
	e_CPU->list.next = 0;
	e_CPU->type = CPU;
	e_CPU->duration = cpu_burst;
	e_CPU->quantum = -1;
	e_CPU->next_prediction = -1;
	e_CPU->timer = 0;
	List_pushBack(&p->events, (ListItem*)e_CPU);

	ProcessEvent* e_IO = (ProcessEvent*) malloc(sizeof(ProcessEvent));
	e_IO->list.prev = 0;
	e_IO->list.next = 0;
	e_IO->type = IO;
	e_IO->duration = io_burst;
	e_IO->quantum = -1;
	e_IO->next_prediction = -1;
	e_IO->timer = 0;
	List_pushBack(&p->events, (ListItem*)e_IO);

	return e_CPU;
};


/* Process_CalculatePrediction() aggiorna i campi duration, quantum e next_predition
	di ogni evento del processo [p], a partire dal processo [start]
*/
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


