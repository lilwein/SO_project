#include "process.h"
#include "linked_list.h"
#pragma once


typedef struct {
	ListItem list;
	int pid;
	ListHead events;

	char usedThisTime;
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
} OS;

typedef struct {
	int core;
} scheduler_args;


void OS_init(OS* os);
void OS_simStep(OS* os);
void OS_destroy(OS* os);

PCB* shortestJobPCB (ListItem* item);

