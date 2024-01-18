#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "os.h"
#include "aux.h"
#include "scheduler_SJF.h"

#define MAX_SIZE_STRING 1024
void enterInLine();

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
		printEscape("7;1;8"); printf("\n\n**********************"); printEscape("28");
		printf("SCHEDULER STATISTICS"); printEscape("8"); 
		printf("**********************"); printEscape("0");
		printf("\n----------------------------------------------------------------\n");

		// Total time
		printEscape("1;48;5;234"); printf("Number of CPUs:\t\t\t\t\t\t%d", core); printEscape("0"); printf("\n");
		printEscape("1;48;5;234"); printf("Number of processes:\t\t\t\t\t%d", os.all_processes.size); printEscape("0"); printf("\n");
		printEscape("1;48;5;234"); printf("Number of CPU and IO bursts:\t\t\t\t%d", os.n_bursts); printEscape("0"); printf("\n");
		printEscape("1;48;5;234"); printf("Total time:\t\t\t\t\t\t%d", timer); printEscape("0"); printf("\n");
		printf("----------------------------------------------------------------\n");

		// Tutti i processi creati o caricati
		printPidList_AUX(&os.all_processes, "All created or loaded processes:\n\t", -2);
		printf("----------------------------------------------------------------\n");
	
		// Waiting time
		double waitingTime = waitingToRun_Time(&os);
		printEscape("1;48;5;234"); printf("Average Waiting Time:\t\t\t\t\t%.2f", waitingTime); printEscape("0");
		printf("\n----------------------------------------------------------------\n");

		// CPU Utilization
		printf("CPU Utilization for each core:\n");
		double tot = 0;
		for(int i=0; i<core; i++){
			double u = ((double) os.CPUs_utilization[i] / (double) timer) * 100;
			tot += u;
			printf("\tCPU %d:\t\t\t\t\t\t%d %%\n", i, (int)u);
		}
		printEscape("1;48;5;234"); printf("Average CPU Utilization:\t\t\t\t"); 
		printf("%d %%", (int) (tot/core)); printEscape("0"); printf("\n");
		printf("----------------------------------------------------------------\n");

		// Throughput
		printEscape("1;48;5;234"); printf("Throughput (processes/time unit):\t\t\t"); printEscape("0");
		printEscape("1;48;5;234"); printf("%.3f", (double)os.all_processes.size / timer); printEscape("0"); printf("\n");
		printf("----------------------------------------------------------------\n\n");
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
}