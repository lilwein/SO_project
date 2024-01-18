#pragma once
#include "linked_list.h"
#include "os.h"

#define handle_error(msg)   do { perror(msg); exit(EXIT_FAILURE); } while (0)

ListItem* List_find_process(ListHead* head, int pid);

int gets_int(int min, int max, char message);
double gets_decay();
int gets_steps();
int gets_last();

double waitingToRun_Time(OS* os);

void setZeroUsed(ListHead* head);

void printPidList_AUX(ListHead* head, char* name, int n);
void printPidListsDebug(OS* os, int n);
void printPidLists(OS* os);

void printEscape(char* str);

void print_message_e(char type);

void changemode(int);
int kbhit(void);