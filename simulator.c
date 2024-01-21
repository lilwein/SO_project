#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "os.h"
#include "aux.h"
#include "scheduler.h"

#define MAX_SIZE_STRING 1024

typedef enum {SJF=0, RR=1} SchedType;
SchedType scheduler;

void enterInLine();
void save_simulation(OS* os, SchedType sched);

OS os;
int timer = 0;
int core;
double decay;


int main(int argc, char** argv) {
	system("clear");
	system("rm -r -f temp");
	system("mkdir temp");

	char* error = "Please choose a scheduler: Shortest Job First or Round Robin.\nRun \"./simulator ['sjf' or 'rr'] [quantum (only for rr)]\"\n\n";
	if(argc>=2) {
		if( !strcmp(argv[1], "sjf") ) {
			scheduler = SJF;
			print_message_e(1);
		}
		else if( !strcmp(argv[1], "rr") ) {
			if(argc!=3) {printf("%s", error); return EXIT_SUCCESS;}
			scheduler = RR;
			print_message_e(0);
		}
		else {printf("%s", error); return EXIT_SUCCESS;}
	}
	else if(argc==1) {
		scheduler = SJF;
		print_message_e(1);
	}

	// Inserimento numero di core
	core = gets_int(1, 16, 12);
	printf("Number of CPUs: "); printEscape("1"); printf("%d\n", core); printEscape("0");

	// Inserimento decay coefficient
	decay = gets_decay();
	// srr_args.core = core;
	printf("Decay Coefficient: "); printEscape("1"); printf("%f\n", decay); printEscape("0");

	// Inizializzazione OS
	OS_init(&os);
	scheduler_args srr_args;
	
	srr_args.core = core;
	if( scheduler == RR ) srr_args.quantum = atoi(argv[2]);

	os.schedule_args = &srr_args;
	
	if( scheduler == SJF ) os.schedule_fn = schedulerSJF;
	if( scheduler == RR ) os.schedule_fn = schedulerRR;
	
	os.CPUs_utilization = (int*) calloc(core, sizeof(int));


	// INSERIMENTO VIA FILE
	char loading_mode;
	print_message_e(2);
	while(1){
		fflush(stdout);
		changemode(1);
		while(!kbhit()){}
		loading_mode = getchar();
		changemode(0);

		// Continue
		if(loading_mode=='\n' || loading_mode=='y' || loading_mode=='n') break;
		// Help Message
		else if(loading_mode=='h'){
			print_message_e(3);
			print_message_e(2);
			continue;
		}
		// Quit
		else if(loading_mode=='q'){
			print_message_e(5);
			return EXIT_SUCCESS;
		}
		else{
			print_message_e(4);
			continue;
		}
	}
	printf("\n");

	// INSERIMENTO VIA FILE
	if(loading_mode=='\n' || loading_mode=='y'){
		// Inserimento filename
		insert_filename: ;

		char* files = (char*) malloc(MAX_SIZE_STRING);
		do{
			print_message_e(8);
			fgets(files, MAX_SIZE_STRING, stdin);
		} while( !strcmp(files, "\n") );
		
		// Processi inseriti correttamente
		int processes_ok = 0;
		
		// Processi non inseriti
		int processes_not_ok = 0;

		// Scansione riga di comando tramite token
		char* token = strtok(files, " \n");
		while(token){
			char* copy = (char*) malloc(strlen(token)+1);
			strcpy(copy, token);
			char* path = "processes/";
			char* extension = ".txt";
			char* filename = (char*) malloc(strlen(token)+strlen(path)+strlen(extension)+1);
			strcpy(filename, path);
			strcat(filename, copy);
			strcat(filename, extension);
			free(copy);
			
			// Inizializzazione processo da file
			Process new_process;
			int num_events = Process_load_file(&new_process, filename);
			// File non trovato
			if(num_events==-1){
				processes_not_ok ++;
				printf("File %s not found\n", filename);
				free(filename);
				token = strtok(NULL, " \n");
				continue;
			}
			// File trovato
			processes_ok ++;
			
			// Calcolo dei quantum predtcion su TUTTI gli eventi
			Process_CalculatePrediction(&new_process, decay, NULL);

			// Ci sono eventi
			if (num_events) {
				Process* new_process_ptr = (Process*)malloc(sizeof(Process));
				*new_process_ptr = new_process;

				// Processo già caricato
				if(List_find_process(&os.processes, new_process_ptr->pid)){
					printf("Process with pid (%d) is already loaded\n", new_process_ptr->pid);
					processes_ok --;
				}
				// Nuovo processo
				else{
					printf("Loading process from %s, pid: %d, arrival time: %d, events: %d\n", filename, new_process.pid, new_process.arrival_time, num_events);
					
					// Inserimento del processo nella lista dei processi
					List_pushBack(&os.processes, (ListItem*)new_process_ptr);

					// Salvataggio del processo in un nuovo file
					Process_save_file(new_process_ptr);
				}
			}
			// Non ci sono eventi
			else printf("Loading process from %s, pid: %d, arrival time: %d, events: %d\n", filename, new_process.pid, new_process.arrival_time, num_events);

			// Prossimo token
			token = strtok(NULL, " \n");

			free(filename);
		}
		free(files);
		printf("\n");

		// Nessun processo inserito
		if(!processes_ok) print_message_e(9);

		// Errore nell'inserimento (file non trovato)
		if(processes_not_ok) {
			printEscape("48;5;234"); printf("%d processes have failed loading. ", processes_not_ok); printEscape("0");
			print_message_e(10);
		}
		// Processo inserito correttamente. Viene chiesto se se ne vogliano inserire altri.
		else print_message_e(11);
		fflush(stdout);

		// Risposta
		char yn;
		while(1){
			changemode(1);
			while(!kbhit()){}
			yn = getchar();
			changemode(0);
			
			// Inserimento di altri processi
			if(yn=='y' || yn=='\n') goto insert_filename;
			// Continua
			else if(yn=='n'){
				break;
			}
			else{
				print_message_e(4);
				continue;
			}
		}

		// Stampa processi inseriti
		printf("\n----------------------------------------------------------------\n");
		printPidList_AUX(&os.processes, "processes", -1);
		printf("----------------------------------------------------------------\n");
	}

	/* ********************************** START OS ********************************** */
	char bar = 1;

	// Numero di salti in avanti
	int steps;

	while(1) {
		if(bar){
			printEscape("8"); printf("\n************************"); printEscape("28");
			printEscape("1;48;5;237");
			printf(" TIME: %08d ", timer); printEscape("0");
			printEscape("8"); printf("*************************"); printEscape("0");
		}

		// Ci sono ancora processi?
		if(!OS_run(&os)) steps = gets_last();
		else steps = gets_steps();


		// Digitato "q": quit
		if(steps==-1) break;

		// Digitato "n" o "p": inserimento nuovi eventi (o processi)
		else if(steps==-2) {
			enterInLine();
			bar = 0;
		}
		// Digitato 0 o "all": si salta alla fine
		else if(steps==0){
			while(OS_run(&os)) OS_simStep(&os, &timer);
		}
		// Digitato il numero di steps in avanti
		else {
			for(int i=0; i<steps; i++){
				if(!OS_run(&os)) break;
				OS_simStep(&os, &timer);
			}
			bar = 1;
		}
	}
	/* ********************************** STOP OS ********************************** */

	// Statistiche dello scheduler
	if(os.all_processes.size){
		printf("\n\n");
		for(int i=0; i<2; i++){
			char e = 1;
			if(i==1){
				e = 0;
				save_simulation(&os, scheduler);
			}

			printEscape_2("7;1;8", e); printf("**********************"); printEscape_2("28", e);
			printf("SCHEDULER STATISTICS"); printEscape_2("8", e); 
			printf("**********************"); printEscape_2("0", e);
			printEscape_2("1", e);
			if( scheduler == SJF ) printf("\nSHORTEST JOB FIRST");
			if( scheduler == RR ) printf("\nROUND ROBIN");
			printEscape_2("0", e);
			printf("\n----------------------------------------------------------------\n");

			// Total time
			printEscape_2("1;48;5;234", e); printf("Number of CPUs:\t\t\t\t\t\t%d", core); printEscape_2("0", e); printf("\n");
			printEscape_2("1;48;5;234", e); printf("Number of processes:\t\t\t\t\t%d", os.all_processes.size); printEscape_2("0", e); printf("\n");
			printEscape_2("1;48;5;234", e); printf("Number of CPU and IO bursts:\t\t\t\t%d", os.n_bursts); printEscape_2("0", e); printf("\n");
			printEscape_2("1;48;5;234", e); printf("Total time:\t\t\t\t\t\t%d", timer); printEscape_2("0", e); printf("\n");
			printf("----------------------------------------------------------------\n");

			// Tutti i processi creati o caricati
			printPidList_AUX(&os.all_processes, "All created or loaded processes:\n\t", -2);
			printf("----------------------------------------------------------------\n");
		
			// Waiting time
			double waitingTime = waitingTime_OS(&os);
			printEscape_2("1;48;5;234", e); printf("Average Waiting Time:\t\t\t\t\t%.2f", waitingTime); printEscape_2("0", e);
			printf("\n----------------------------------------------------------------\n");

			// Turnaround time
			double turnaroundTime = turnaroundTime_OS(&os);
			printEscape_2("1;48;5;234", e); printf("Average Turnaround Time:\t\t\t\t%.2f", turnaroundTime); printEscape_2("0", e);
			printf("\n----------------------------------------------------------------\n");

			// CPU Utilization
			printf("CPU Utilization for each core:\n");
			double tot = 0;
			for(int i=0; i<core; i++){
				double u = ((double) os.CPUs_utilization[i] / (double) timer) * 100;
				tot += u;
				printf("\tCPU %d:\t\t\t\t\t\t%d %%\n", i, (int)u);
			}
			printEscape_2("1;48;5;234", e); printf("Average CPU Utilization:\t\t\t\t"); 
			printf("%d %%", (int) (tot/core)); printEscape_2("0", e); printf("\n");
			printf("----------------------------------------------------------------\n");

			// Throughput
			printEscape_2("1;48;5;234", e); printf("Throughput (processes/time unit):\t\t\t"); printEscape_2("0", e);
			printEscape_2("1;48;5;234", e); printf("%.3f", (double)os.all_processes.size / timer); printEscape_2("0", e); printf("\n");
			printf("----------------------------------------------------------------\n\n");
			
			if(i==1) freopen("/dev/tty", "w", stdout); // LINUX
			// if(i==1) freopen("CON", "w", stdout); // WINDOWS
			#undef _NO_ANSI
		}
	}

	print_message_e(5);
	return EXIT_SUCCESS;
}


// INSERIMENTO NUOVI EVENTI
void enterInLine(){

	while(1){
		Process* p;

		// Si chiede da terminale il pid del processo a cui si vogliono aggiungere eventi
		int pid = gets_int(0, 9999, 16);
		if(pid==0) break;
		
		// Non si possono aggiungere eventi a processi in waiting
		ListItem* aux = List_find_process(&os.waiting, pid);
		if(aux) { print_message_e(14); continue; }

		// Il processo esiste?
		aux = List_find_process(&os.running, pid);
		if(!aux) aux = List_find_process(&os.processes, pid);
		if(!aux) aux = List_find_process(&os.ready, pid);
		
		// Il processo con il pid inserito NON ESISTE
		if(!aux){

			// Processo non esistente ma esistito in passato
			aux = List_find_process(&os.all_processes, pid);
			if(aux){
				printf("\n"); printEscape("4;48;5;234");
				printf("Process with pid (%d) is already existed in the past. Please choose a different pid or select an existing process.", pid);
				printEscape("0"); printf("\n");
				continue;
			}

			// Processo non esistente: creare nuovo processo
			Process new_process;
			Process_init_inline(&new_process, pid, timer);

			Process* new_process_ptr = (Process*)malloc(sizeof(Process));
			*new_process_ptr = new_process;
			p = new_process_ptr;

			// Inserimento da terminale del CPU e IO Burst
			int cpu_burst = gets_int(1, 9999, 17);
			int io_burst = gets_int(1, 9999, 18);
			
			// Aggiunta dei due eventi al processo
			Process_load_inline(new_process_ptr, cpu_burst, io_burst);

			// Calcolo dei quantum predtcion su TUTTI gli eventi
			Process_CalculatePrediction(new_process_ptr, decay, NULL);

			printf("Loading new process with pid: %d, arrival time: %d\nLoading new event with CPU_BURST: %d, IO_BURST: %d\n", new_process.pid, new_process.arrival_time, cpu_burst, io_burst);
			printf("Process with pid (%d) has %d events\n", new_process_ptr->pid, new_process_ptr->events.size);
			List_pushBack(&os.processes, (ListItem*)new_process_ptr);

			// Salvataggio del processo in un nuovo file
			Process_save_file(p);
		}
		// PROCESSO ESISTE
		else {
			Process* existing_process = (Process*) aux;
			p = existing_process;

			// Inserimento da terminale del CPU e IO Burst
			int cpu_burst = gets_int(1, 9999, 17);
			int io_burst = gets_int(1, 9999, 18);
			
			// Aggiunta dei due eventi al processo: new_event punta al primo evento (CPU)
			ProcessEvent* new_event = Process_load_inline(existing_process, cpu_burst, io_burst);

			// Calcolo dei quantum predtcion A PARTIRE DALL'EVENTO NEW_EVENT
			Process_CalculatePrediction(existing_process, decay, new_event);

			printf("Loading new event with CPU_BURST: %d, IO_BURST: %d\n", cpu_burst, io_burst);
			printf("Process with pid (%d) has now %d events\n", existing_process->pid, existing_process->events.size);

			// Scrittura dei nuovi eventi nel file già creato
			Event_save_file(p, cpu_burst, io_burst);
		}

		break;
	}
};

void save_simulation(OS* os, SchedType sched){

	char* path = (char*) malloc(MAX_SIZE_STRING);
	strcpy(path, "./output/");
	char core_str[2]; sprintf(core_str, "%d", core);
	strcat(path, core_str);
	strcat(path, "-cpus");

	char** procs_array = (char**) malloc(os->all_processes.size*sizeof(char*));
	int k = 0;

	ListItem* aux = os->all_processes.first;
	while(aux){
		PCB* pcb = (PCB*) aux;

		char* proc = (char*) malloc(MAX_SIZE_STRING);
		strcat(proc, "p");
		char pid_str[PROC_MAX_LENGHT]; sprintf(pid_str, "%d", pcb->pid);
		strcat(proc, pid_str);

		strcat(path, "_");
		strcat(path, proc);

		procs_array[k++] = proc;

		aux = aux->next;
	}
	char* mkdir = (char*) malloc(10+strlen(path));
	strcpy(mkdir, "mkdir -p ");
	strcat(mkdir, path);
	system(mkdir);

	strcat(path, "/");
	
	for(int i=0; i<os->all_processes.size; i++){
		char* mv = (char*) malloc(18+strlen(procs_array[i])+strlen(path));
		sprintf(mv, "mv ./temp/%s.txt %s", procs_array[i], path);
		system(mv);

		free(procs_array[i]);
	}

	if( scheduler == SJF ) strcat(path, "SJF_");
	if( scheduler == RR ) strcat(path, "RR_");
	strcat(path, "statistics.txt");

	freopen(path, "w+", stdout);

	free(path);
	free(mkdir);

	free(procs_array);
}