#pragma once
#include "linked_list.h"
#include "os.h"

ListItem* List_find_process(ListHead* head, ListItem* item);

void printPidLists(OS* os);
void printPidList_AUX(ListHead* head, char* name, int n);
void printPidListsDebug(OS* os, int n);
void setZeroUsed(ListHead* head);
void printUsed(OS* os, char* name, int n);

void printEscape(char* str);

void changemode(int);
int kbhit(void);