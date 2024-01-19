#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>

#include "aux.h"
#include "os.h"

#define MAX_BUFFER 1024


/* List_find_process() cerca nella lista il processo con pid [pid] e lo restituisce se lo trova,
	altrimenti restituisce NULL */
ListItem* List_find_process(ListHead* head, int pid) {
	ListItem* aux = head->first;
	while(aux){
		Process* p = (Process*) aux;
		if (p->pid==pid)
			return aux;
		aux=aux->next;
	}
  return NULL;
};


/* gets_int() prende il valore inserito da terminale e controlla se è un intero compreso tra [min] e [max].
	In tal caso restituisce l'intero digitato, altrimenti continua a richiedere da terminale */
int gets_int(int min, int max, char message){
	int lenght = MAX_BUFFER;
	char* string = (char*) malloc(lenght+1);
	int number;
	char ok = 1;
	while(ok){
		print_message_e(message);
		// fflush(stdout);

		fgets(string, lenght, stdin);
		for(int i=0; i<strlen(string)-1; i++){
			if(!isdigit(string[i])){
				printf("Invalid value, please insert a number\n");
				ok = 0;
				break;
			}
		}
		if(!ok){
			ok = 1;
			continue;
		}
		number = atoi(string);
		if(number<min || number>max){
			printf("Invalid number, please try again\n");
			continue;
		}
		ok = 0;
	}
	return number;
};


/* gets_decay() prende il valore inserito da terminale e controlla se è un numero compreso tra 0 e 1.
	In tal caso restituisce il double digitato, altrimenti continua a richiedere da terminale */
double gets_decay(){
	int lenght = MAX_BUFFER;
	char* string = (char*) malloc(lenght);
	double number;
	char ok = 1;
	while(ok){
		int dots = 0;
		print_message_e(15);
		// fflush(stdout);

		fgets(string, lenght, stdin);
		if(!strcmp(string, "\n")) return 0.5;
		for(int i=0; i<strlen(string)-1; i++){
			if(string[i]=='.') dots++;
			if(!isdigit(string[i]) && dots!=1){
				printf("Invalid value, please insert a number\n");
				ok = 0;
				break;
			}
		}
		if(!ok){
			ok = 1;
			continue;
		}
		number = strtod(string, NULL);
		if(number<0 || number>1){
			printf("Invalid number, please try again\n");
			continue;
		}
		ok = 0;
	}
	return number;
};


/* gets_steps() prende il valore inserito da terminale.
	Se è stato digitato "all", "q", "n" o "p", restituisce un certo valore.
	Se è stato digitato ENTER, restituisce 1.
	Se è stato digitato un intero controlla che non sia negativo e, in tal caso, lo restiutisce.
	Altrimenti continua a richiedere da terminale. */
int gets_steps(){
	int lenght = MAX_BUFFER;
	char* string = (char*) malloc(lenght);
	int number;
	char ok = 1;
	while(ok){
		print_message_e(13);
		fflush(stdout);

		fgets(string, lenght, stdin);
		if(!strcmp(string, "all\n")) return 0;
		if(!strcmp(string, "\n")) return 1;
		if(!strcmp(string, "q\n")) return -1;
		if(!strcmp(string, "n\n") || !strcmp(string, "p\n")) return -2;
		for(int i=0; i<strlen(string)-1; i++){
			if(!isdigit(string[i])){
				printf("Invalid value, please insert a number\n");
				ok = 0;
				break;
			}
		}
		if(!ok){
			ok = 1;
			continue;
		}
		number = atoi(string);
		if(number<0){
			printf("Invalid number, please try again\n");
			continue;
		}
		ok = 0;
	} 
	return number;
};


/* gets_last() prende il valore inserito da terminale.
	Se è stato digitato "q", "n" o "p", restituisce un certo valore.
	Altrimenti continua a richiedere da terminale. */
int gets_last(){
	int lenght = MAX_BUFFER;
	char* string = (char*) malloc(lenght);
	print_message_e(19);
	while(1){
		print_message_e(20);
		fflush(stdout);

		fgets(string, lenght, stdin);
		if(!strcmp(string, "q\n")) return -1;
		if(!strcmp(string, "n\n") || !strcmp(string, "p\n")) return -2;
	} 
	return 0;
};


// waitingTime_OS() stampa a schermo la waiting time di ogni processo e ne restituisce la media
double waitingTime_OS(OS* os){
	ListItem* aux = os->all_processes.first;
	double time = 0;
	if(aux) printf("Time that processes spent waiting to be run:\n");
	while(aux){
		PCB* pcb = (PCB*) aux;
		time += pcb->waitingTime;
		printf("\tprocess (%d):\t\t\t\t\t%d\n", pcb->pid, pcb->waitingTime);
		aux = aux->next;
	}
	return time / os->all_processes.size;
};


// setZeroUsed() imposta a 0 il campo "usedThisTime" di tutti i processi della lista
void setZeroUsed(ListHead* head){
	ListItem* item = head->first;
	while(item){
		PCB* pcb = (PCB*) item;
		pcb->usedThisTime = 0;
		item = item->next;
	}
};


// FUNZIONI AUSILIARE
int printUsed_AUX(ListHead* head){
	int r = 0;
	ListItem* item = head->first;
	while(item){
		PCB* pcb = (PCB*) item;
		if(pcb->usedThisTime == 1){
			printf("(%d) ", pcb->pid);
			r = 1;
		}
		item = item->next;
	}
	return r;
};
void printUsed(OS* os, char* name, int n){
	printf("%s %d:\t", name, n);
	if (	printUsed_AUX(&os->running) +
			printUsed_AUX(&os->waiting) +
			printUsed_AUX(&os->ready) == 0
	) printf("-");
};
void printPidList_AUX(ListHead* head, char* name, int n){
	ListItem* item = head->first;
	if(n==-2) printf("%s", name);
	else if(n==-1) printf("%s:\t", name);
	else printf("%s %d:\t", name, n);
	if(!item){
		printf(" -\n");
		return;
	}
	while(item){
		PCB* pcb = (PCB*) item;
		printf("(%d) ", pcb->pid);
		item = item->next;
	}
	printf("\n");
};
void printPidListsDebug(OS* os, int n){
	printf("+++++ LISTS %d +++++\n", n);
	printPidList_AUX(&os->running, "running", n);
	printPidList_AUX(&os->ready, "ready   ", n);
	printPidList_AUX(&os->waiting, "waiting", n);
	printUsed(os, "usedThisTime", n);
	printf("\n");
};

// printPidLists() stampa a schermo tutti i processi per ogni lista dell'OS
void printPidLists(OS* os){
	printPidList_AUX(&os->running, "running", -1);
	printPidList_AUX(&os->ready, "ready   ", -1);
	printPidList_AUX(&os->waiting, "waiting",-1);
	// printPidList_AUX(&os->processes, "processes",-1);
};


/* printEscape inserisce [str] tra i caratteri di escape "\e[" e "m",
	se _NO_ANSI non è definito. */
void printEscape(char* str){
	#ifdef _NO_ANSI
		return;
	#endif
	printf("\e[%sm", str);
}


// Funzione ausiliaria di stampa a schermo
void print_message_e(char type){
	if(type==1){
		// 1: welcome
		char* message = "\nWelcome to CPU Scheduler Simulator: Preemptive scheduler with Shortest Job First\n";
		printEscape("1;4;7");	printf("%s", message); printEscape("0");
	}
	else if(type==2){
		// 2: file
		printf("\n"); printEscape("48;5;234");
		printf("Do you want to load processes from file? (");
		printEscape("1;3"); printf("ENTER"); printEscape("22;23");
		printf(" or "); 
		printEscape("1;3"); printf("y"); printEscape("22;23");
		printf(" for yes, "); 
		printEscape("1;3"); printf("n"); printEscape("22;23");
		printf(" for no, "); 
		printEscape("1;3"); printf("h"); printEscape("22;23");
		printf(" for help, "); 
		printEscape("1;3"); printf("q"); printEscape("22;23");
		printf(" for quit)");
		printEscape("0"); printf("\n");
	}
	else if(type==3){
		// 3: help
		printf("\n");
		printEscape("4"); printf("Loading from file:"); printEscape("24");
		printf(" the simulator will load the processes written in txt file in \"processes\" folder. Txt file should be like:\n\n\tPROCESS\t\t[id] [arrival time]\n\tCPU_BURST\t[duration]\n\tIO_BURST\t[duration]\n\t...\t\t...\n\nFile must terminate with a IO_BURST line.\n\n");
		printEscape("24");
		printf("However, you can load processes into the OS (or events into a process) at any time\n");
	}
	else if(type==4){
		// 4: invalid button
		printf("Invalid button, please try again\n");
	}
	else if(type==5){
		// 5: goodbye
		printEscape("1;4;7"); printf("Thank you for using CPU Scheduler Simulator!"); printEscape("0"); printf("\n");
	}
	else if(type==8){
		// 8: insert processes
		printEscape("48;5;234"); printf("Insert txt files contained in ");
		printEscape("3"); printf("processes"); printEscape("23");
		printf(" folder, without extension (ex: p1 p2 p3):"); printEscape("0"); printf(" ");
	}
	else if(type==9){
		// 9: no process loaded
		printEscape("48;5;234"); printf("No process has ben loaded. "); printEscape("0");
	}
	else if(type==10 || type==11){
		char* message;
		// 10: try again
		if(type==10) message = "Do you want to try again? (";
		// 11: nuovi processi
		if(type==11) message = "Do you want to load more processes from file? (";

		printEscape("48;5;234"); printf("%s", message);
		printEscape("1;3"); printf("ENTER"); printEscape("22;23");
		printf(" or "); 
		printEscape("1;3"); printf("y"); printEscape("22;23");
		printf(" for yes, ");
		printEscape("1;3"); printf("n"); printEscape("22;23");
		printf(" for no)");
		printEscape("0"); printf("\n");
	}
	else if(type==12){
		// 12: get_core
		printf("\n"); printEscape("48;5;234"); printf("Insert number of available CPUs:");
		printEscape("0"); printf(" ");
	}
	else if(type==13){
		// 13: get_steps
		printf("\n"); printEscape("48;5;234"); printf("Press ");
		printEscape("1;3"); printf("n"); printEscape("22;23");
		printf(" or ");
		printEscape("1;3"); printf("p"); printEscape("22;23");
		printf(" to CREATE a new process to or ADD events to a ");
		printEscape("4"); printf("running"); printEscape("24");
		printf(" or ");
		printEscape("4"); printf("ready"); printEscape("24");
		printf(" process."); printEscape("0"); printf("\n");
		printEscape("48;5;234");
		printf("Otherwise, TYPE how many steps you want to go forward (");
		printEscape("1;3"); printf("0"); printEscape("22;23"); printf(" or ");
		printEscape("1;3"); printf("all"); printEscape("22;23"); printf(" for skip to end, ");
		printEscape("1;3"); printf("ENTER"); printEscape("22;23"); printf(" for one step, ");
		printEscape("1;3"); printf("q"); printEscape("22;23"); printf(" for quit)");
		printEscape("0"); printf(" ");
	}
	else if(type==14){
		// 14: avviso: aggiungere eventi solo ai processi ready o running
		printf("\n"); printEscape("4;48;5;234");
		printf("Please add events ONLY to ");
		printEscape("1;3"); printf("running"); printEscape("22;23");
		printf(" or ");
		printEscape("1;3"); printf("ready"); printEscape("22;23");
		printf(" process.");
		printEscape("0"); printf("\n");
	}
	else if(type==15){
		// 15: gets_decay
		printf("\n"); printEscape("48;5;234");
		printf("Decay Coefficient ( 0 <= x <= 1 ) is set on 0.5. ");
		printf("Insert a new value or press ");
		printEscape("1;3"); printf("ENTER"); printEscape("22;23");
		printf(" to continue");
		printEscape("0"); printf(" ");
	}
	else if(type==16){
		// 16: insert pid
		printf("\n"); printEscape("48;5;234");
		printf("Insert pid (number) of the process to which the events are added (process will be created if it doesn't exist), or type ");
		printEscape("1;3"); printf("0"); printEscape("22;23");
		printf(" to cancel:");
		printEscape("0"); printf(" ");
	}
	else if(type==17){
		// 16: insert cpu burst
		printEscape("48;5;234");
		printf("Insert CPU BURST:");
		printEscape("0"); printf(" ");
	}
	else if(type==18){
		// 16: insert io burst
		printEscape("48;5;234");
		printf("Insert IO BURST:");
		printEscape("0"); printf(" ");
	}
	else if(type==19){
		// 19: gets_last
		printf("\n\n"); printEscape("1;7;48;5;234");
		printf("There are no processes in the OS."); 
		printEscape("0"); printf("\n");
		
	}
	else if(type==20){
		// 20: gets_last
		printEscape("48;5;234");
		printf("Press ");
		printEscape("1;3"); printf("n"); printEscape("22;23");
		printf(" or ");
		printEscape("1;3"); printf("p"); printEscape("22;23");
		printf(" to CREATE a new process or ");
		printEscape("1;3"); printf("q"); printEscape("22;23");
		printf(" for quit."); printEscape("0"); printf(" ");
	}
	else if(type==21){
		
	}
	else assert(0 && "invalid argument");

};


void changemode(int dir){
  	static struct termios oldt, newt;

  	if ( dir == 1 ){
		tcgetattr( STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~( ICANON | ECHO );
		tcsetattr( STDIN_FILENO, TCSANOW, &newt);
	}
	else
		tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

int kbhit (void){
	struct timeval tv;
	fd_set rdfs;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&rdfs);
	FD_SET (STDIN_FILENO, &rdfs);

	select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &rdfs);
}