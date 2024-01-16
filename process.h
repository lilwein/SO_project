#pragma once
#include "linked_list.h"

typedef enum {CPU=0, IO=1} ResourceType;

typedef struct {
	ListItem list;
	ResourceType type;
	int duration;
	int quantum;
	int next_prediction;
} ProcessEvent;

typedef struct {
	ListItem list;
	int pid;
	int arrival_time;
	ListHead events;
} Process;

int Process_load_file(Process* p, const char* filename);
int Process_save_file(const Process* p, const char* filename);

void Process_init_inline(Process* p, int pid, int arrival);
void Process_load_inline(Process* p, int cpu_burst, int io_burst);

void Process_CalculatePrediction(Process* p, double decay);