#include "process.h"
#include "linked_list.h"
#include "os.h"
#pragma once

void printPidLists(OS* os);

void printPidListsDebug(OS* os, int n);
void setZeroUsed(ListHead* head);
void printUsed(OS* os, char* name, int n);

void printEscape(char* str);