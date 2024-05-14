#pragma once


void schedulerSJF(OS* os, void* args_);
void SJF_calculatePrediction(PCB* pcb, void* args_);

PCB* shortestJobPCB (ListItem* item);

void schedulerRR(OS* os, void* args_);