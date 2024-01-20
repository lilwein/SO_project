#pragma once


void schedulerSJF(OS* os, void* args_);

PCB* shortestJobPCB (ListItem* item);

void schedulerRR(OS* os, void* args_);