#pragma once
#include "linked_list.h"

typedef enum {CPU=0, IO=1} ResourceType;

// evento di un processo
typedef struct {
	ListItem list;
	ResourceType type;
	int duration;
	int prediction;
	int next_prediction;
} ProcessEvent;

// processo
typedef struct {
	ListItem list;
	int pid;
	int arrival_time;
	ListHead events;
} Process;

void Process_CalculatePrediction(Process* p);

int Process_load(Process* p, const char* filename);

int Process_save(const Process* p, const char* filename);
