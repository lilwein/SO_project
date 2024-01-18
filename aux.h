#pragma once
#include "linked_list.h"
#include "os.h"

ListItem* List_find_process(ListHead* head, int pid);

int gets_int(int min, int max, char message);
int gets_steps();
int gets_last();
double gets_decay();

void print_message_e(char type);

double waitingToRun_Time(OS* os);

void printPidLists(OS* os);
void printPidList_AUX(ListHead* head, char* name, int n);
void printPidListsDebug(OS* os, int n);
void setZeroUsed(ListHead* head);
void printUsed(OS* os, char* name, int n);

void printEscape(char* str);

void changemode(int);
int kbhit(void);