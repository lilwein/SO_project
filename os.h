#pragma once
#include "linked_list.h"
#include "process.h"

typedef struct {
	ListItem list;
	int pid;
	ListHead events;

	char usedThisTime;
	int waitingToRun;
} PCB;

typedef struct {
	ListItem list;
	int pid;
	char usedThisTime;
	int waitingToRun;
} Short_PCB;

struct OS;
typedef void (*ScheduleFn)(struct OS* os, void* args);

typedef struct OS{
	ListHead running;
	ListHead ready;
	ListHead waiting;
	int timer;
	ScheduleFn schedule_fn;
	void* schedule_args;

	ListHead processes;
	ListHead all_processes;
} OS;

typedef struct {
	int core;
} scheduler_args;

void OS_init(OS* os);
void OS_simStep(OS* os, int* timer);
void OS_destroy(OS* os);

int OS_run(OS* os);