#pragma once
#include "linked_list.h"
#include "process.h"

typedef struct {
	ListItem list;
	int pid;
	ListHead events;

	char usedThisTime;
	int waitingTime;
	int turnaroundTime;
} PCB;

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
	int* CPUs_utilization;
	int n_bursts;
} OS;

typedef struct {
	int core;
	int quantum;
} scheduler_args;

void OS_init(OS* os);
void OS_simStep(OS* os, int* timer);
int OS_run(OS* os);

PCB* PCB_copy(PCB* src);

double waitingTime_OS(OS* os);
double turnaroundTime_OS(OS* os);
void turnaroundTime_inc(OS* os);

void setZeroUsed(ListHead* head);
