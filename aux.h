#pragma once
#include "linked_list.h"
#include "os.h"

ListItem* List_find_process(ListHead* head, ListItem* item);

int gets_core(int lenght, char* message);
int gets_steps(int lenght, char* message);

void printPidLists(OS* os);
void printPidList_AUX(ListHead* head, char* name, int n);
void printPidListsDebug(OS* os, int n);
void setZeroUsed(ListHead* head);
void printUsed(OS* os, char* name, int n);

void printEscape(char* str);

void changemode(int);
int kbhit(void);