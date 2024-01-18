#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "os.h"
#include "aux.h"
#include "scheduler_SJF.h"

#define MAX_SIZE_STRING 1024

int enterInLine();

OS os;
int timer = 0;

int core;
double decay;

int main(int argc, char** argv) {
	system("clear");

	print_message_e(1);

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
	os.schedule_args = &srr_args;
	os.schedule_fn = schedulerSJF;
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

		if(loading_mode=='\n' || loading_mode=='y' || loading_mode=='n') break;
		else if(loading_mode=='h'){
			print_message_e(3);
			print_message_e(2);
			continue;
		}
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
	if(loading_mode=='\n' || loading_mode=='y'){
		// Inserimento filename
		insert_filename: ;
		char* files = (char*) malloc(MAX_SIZE_STRING);
		do{
			print_message_e(8);
			fgets(files, MAX_SIZE_STRING, stdin);
		} while( !strcmp(files, "\n") );
		// printf("\n");
		
		int processes_ok = 0;
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
			
			// Inizializzazione processo
			Process new_process;
			int num_events = Process_load_file(&new_process, filename);
			if(num_events==-1){
				processes_not_ok ++;
				printf("File %s not found\n", filename);
				free(filename);
				token = strtok(NULL, " \n");
				continue;
			}
			processes_ok ++;
			
			// Calcolo dei quantum predtcion
			Process_CalculatePrediction(&new_process, decay, NULL);

			// Se ci sono eventi, inserimento del processo nella lista dei processi
			if (num_events) {
				Process* new_process_ptr = (Process*)malloc(sizeof(Process));
				*new_process_ptr = new_process;
				if(List_find_process(&os.processes, new_process_ptr->pid)){
					printf("Process with pid (%d) is already loaded\n", new_process_ptr->pid);
					processes_ok --;
				}
				else{
					printf("Loading process from %s, pid: %d, arrival time: %d, events: %d\n", filename, new_process.pid, new_process.arrival_time, num_events);
					List_pushBack(&os.processes, (ListItem*)new_process_ptr);
				}
			}
			else printf("Loading process from %s, pid: %d, arrival time: %d, events: %d\n", filename, new_process.pid, new_process.arrival_time, num_events);

			free(filename);
			token = strtok(NULL, " \n");
		}
		free(files);

		// Nessun processo inserito o errore nell'inserimento
		printf("\n");
		if(!processes_ok) print_message_e(9);
		if(processes_not_ok) {
			printEscape("48;5;234"); printf("%d processes have failed loading. ", processes_not_ok); printEscape("0");
			print_message_e(10);
		}
		else print_message_e(11);
		fflush(stdout);
		char yn;
		while(1){
			changemode(1);
			while(!kbhit()){}
			yn = getchar();
			changemode(0);

			if(yn=='y' || yn=='\n') goto insert_filename;
			else if(yn=='n'){
				break;
			}
			else{
				print_message_e(4);
				continue;
			}
		}

		printf("\n----------------------------------------------------------------\n");
		printPidList_AUX(&os.processes, "processes", -1);
		printf("----------------------------------------------------------------\n");
	}

	// START
	char bar = 1;
	int steps;

	while(1) {
		if(bar){
			printEscape("8"); printf("\n************************"); printEscape("28");
			printEscape("1;48;5;237");
			printf(" TIME: %08d ", timer); printEscape("0");
			printEscape("8"); printf("*************************"); printEscape("0");
		}

		if(!OS_run(&os)) steps = gets_last();
		else steps = gets_steps();

		if(steps==-1) break;
		if(steps==-2) {
			if(!enterInLine()) return EXIT_SUCCESS;
			bar = 0;
		}
		else if(steps==0){
			while(OS_run(&os)) OS_simStep(&os, &timer);
		}
		else {
			for(int i=0; i<steps; i++){
				if(!OS_run(&os)) break;
				OS_simStep(&os, &timer);
			}
			bar = 1;
		}
	}
	// STOP

	// SCHEDULER STATISTICS
	printEscape("7;1;8"); printf("\n\n**********************"); printEscape("28");
	printf("SCHEDULER STATISTICS"); printEscape("8"); 
	printf("**********************"); printEscape("0");
	printf("\n----------------------------------------------------------------\n");

	// Total time
	printEscape("1;48;5;234");
	printf("Total time:\t\t\t%d", timer); printEscape("0");
	printf("\n----------------------------------------------------------------\n");

	// Tutti i processi creati o caricati
	printPidList_AUX(&os.all_processes, "All created or loaded processes:\n\t", -2);
	printf("----------------------------------------------------------------\n");

	// Waiting time
	double waitingTime = waitingToRun_Time(&os);
	printEscape("1;48;5;234"); printf("Average Waiting Time:\t%.2f", waitingTime); printEscape("0");
	printf("\n----------------------------------------------------------------\n");

	// CPU Utilization
	printEscape("1;48;5;234"); printf("CPU Utilization for each core:"); printEscape("0"); printf("\n");
	for(int i=0; i<core; i++){
		double u = (double) os.CPUs_utilization[i] / (double) timer;
		// printf("\tCPU %d: utiliz: %d, %f\n", i, os.CPUs_utilization[i], u);
		printf("\tCPU %d:\t\t\t%d %%\n", i, (int)(u*100) );
	}
	printf("----------------------------------------------------------------\n\n");



	print_message_e(5);
	return EXIT_SUCCESS;
}


int enterInLine(){

	while(1){
		Process* p;
		int pid = gets_int(0, 9999, 16);
		if(pid==0) break;
		
		ListItem* aux = List_find_process(&os.waiting, pid);
		if(aux) { print_message_e(14); continue; }

		aux = List_find_process(&os.running, pid);
		if(!aux) aux = List_find_process(&os.processes, pid);
		if(!aux) aux = List_find_process(&os.ready, pid);
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

			int cpu_burst = gets_int(1, 9999, 17);
			int io_burst = gets_int(1, 9999, 18);
			
			Process_load_inline(new_process_ptr, cpu_burst, io_burst);

			// Calcolo dei quantum predtcion
			Process_CalculatePrediction(new_process_ptr, decay, NULL);

			printf("\nLoading new process with pid: %d, arrival time: %d\nLoading new event with CPU_BURST: %d, IO_BURST: %d\n", new_process.pid, new_process.arrival_time, cpu_burst, io_burst);
			printf("\nProcess with pid (%d) has %d events\n", new_process_ptr->pid, new_process_ptr->events.size);
			List_pushBack(&os.processes, (ListItem*)new_process_ptr);
		}
		else {
			// Processo esistente
			Process* existing_process = (Process*) aux;
			p = existing_process;

			int cpu_burst = gets_int(1, 9999, 17);
			int io_burst = gets_int(1, 9999, 18);
					
			ProcessEvent* new_event = Process_load_inline(existing_process, cpu_burst, io_burst);

			// Process_save_file(p, "1");

			// Calcolo dei quantum predtcion
			Process_CalculatePrediction(existing_process, decay, new_event);

			printf("\nProcess with pid: %d already exists\nLoading new event with CPU_BURST: %d, IO_BURST: %d\n", existing_process->pid, cpu_burst, io_burst);
			printf("\nProcess with pid (%d) has %d events\n", existing_process->pid, existing_process->events.size);
			
			printf("%d\n", ((ProcessEvent*) existing_process->events.last->prev)->duration );
		}
		
		char pid_str[5];
		sprintf(pid_str, "%d", pid);
		Process_save_file(p, pid_str);

		break;
	}
	// 
	return 1;
}