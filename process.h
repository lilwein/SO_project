#pragma once
#include "linked_list.h"

#define PROC_MAX_LENGHT 5

typedef enum {CPU=0, IO=1} ResourceType;

typedef struct {
	ListItem list;
	ResourceType type;

	int duration;
	double quantum;
	double next_prediction;
	
	int timer;
} ProcessEvent;

typedef struct {
	ListItem list;
	int pid;
	int arrival_time;
	ListHead events;
} Process;

int Process_load_file(Process* p, const char* filename);
int Process_save_file(const Process* p);
void Event_save_file(const Process* p, int cpu_burst, int io_burst);

void Process_init_inline(Process* p, int pid, int arrival);
ProcessEvent* Process_load_inline(Process* p, int cpu_burst, int io_burst);

void Process_CalculatePrediction(Process* p, double decay, ProcessEvent* start);