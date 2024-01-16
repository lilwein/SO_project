#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "os.h"
#include "aux.h"
#include "scheduler_SJF.h"

#define MAX_SIZE_STRING 1024


void print_message_e(char type){
	if(type==1){
		// 1: welcome
		char* message = "\nWelcome to CPU Scheduler Simulator: Preemptive scheduler with Shortest Job First\n\n";
		printEscape("1;7");	printf("%s", message); printEscape("0");
	}
	else if(type==2){
		// 2: press
		char* message_1 = "Press ";
		char* message_2 = " for loading processes from file or ";
		char* message_3 = " for loading processes in line (";
		char* message_4 = " for help, ";
		char* message_5 = " for quit)";
		printEscape("48;5;234"); printf("%s", message_1); printEscape("1;3"); printf("f"); printEscape("22;23"); printf("%s", message_2); 
		printEscape("1;3"); printf("i"); printEscape("22;23"); printf("%s", message_3); 
		printEscape("1;3"); printf("h"); printEscape("22;23"); printf("%s", message_4); 
		printEscape("1;3"); printf("q"); printEscape("22;23"); printf("%s", message_5); printEscape("0"); printf("\n\n");
	}
	else if(type==3){
		// 3: help
		char* message_1 = "Loading from file:";
		char* message_2 = " the simulator will load the processes written in txt file in \"processes\" folder. Txt file should be like:\n\n\tPROCESS\t\t[id] [arrival time]\n\tCPU_BURST\t[duration]\n\tIO_BURST\t[duration]\n\t...\t\t...\n\nFile must terminate with a IO_BURST line.\n\n";
		char* message_3 = "Loading in line:";
		char* message_4 = " the simulator will load the processes when you ask for it.\n\n";
		printEscape("4"); printf("%s", message_1); printEscape("24");
		printf("%s", message_2); printEscape("24");
		printEscape("4"); printf("%s", message_3); printEscape("24");
		printf("%s", message_4);
	}
	else if(type==4){
		// 4: invalid button
		printf("Invalid button, please try again\n");
	}
	else if(type==5){
		// 5: goodbye
		printEscape("1;7"); printf("Thank you for using CPU Scheduler Simulator!"); printEscape("0"); printf("\n");
	}
	else if(type==6){
		// 6: loading from file
		printEscape("1;7"); printf("LOADING FROM FILE"); printEscape("0"); printf("\n");
	}
	else if(type==7){
		// 7: loading in line
		printEscape("1;7"); printf("LOADING IN LINE"); printEscape("0"); printf("\n");
	}
	else if(type==8){
		// 8: insert processes
		printEscape("48;5;234"); printf("Insert txt files contained in ");
		printEscape("3"); printf("processes"); printEscape("23");
		printf(" folder (ex: p1 p2 p3):"); printEscape("0"); printf(" ");
	}
	else if(type==9){
		// 9: no process loaded
		printEscape("48;5;234"); printf("No process has ben loaded. "); printEscape("0");
	}
	else if(type==10){
		// 10: try again
		char* message = "Do you want to try again? (";
		printEscape("48;5;234"); printf("%s", message);
		printEscape("1;3"); printf("y"); printEscape("22;23"); printf(" for yes, ");
		printEscape("1;3"); printf("n"); printEscape("22;23"); printf(" for no)"); printEscape("0");
		printf("\n");
	}
	else if(type==11){
		// 11: nuovi processi
		char* message = "Do you want to load more processes? (";
		printf("\n"); printEscape("48;5;234"); printf("%s", message);
		printEscape("1;3"); printf("y"); printEscape("22;23"); printf(" for yes, ");
		printEscape("1;3"); printf("n"); printEscape("22;23"); printf(" for no)"); printEscape("0");
		printf("\n");
	}
	else if(type==12){

	}
	else assert(0 && "invalid argument");

}


OS os;

int main(int argc, char** argv) {
	print_message_e(1);
	print_message_e(2);

	// Modalità di inserimento dei processi
	int loading_mode;
	while(1){
		fflush(stdout);
		changemode(1);
		while(!kbhit()){}
		loading_mode = getchar();
		changemode(0);

		if(loading_mode=='f' || loading_mode=='i') break;
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

	// Inserimento via file
	if(loading_mode=='f'){
		print_message_e(6);
		
		// Inizializzazione OS
		OS_init(&os);
		scheduler_args srr_args;
		os.schedule_args = &srr_args;
		os.schedule_fn = schedulerSJF;

		// Inserimento filename
		insert_filename: ;
		char* files = (char*) malloc(MAX_SIZE_STRING);
		do{
			print_message_e(8);
			fgets(files, MAX_SIZE_STRING, stdin);
		} while( !strcmp(files, "\n") );
		printf("\n");
		
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
			Process_CalculatePrediction(&new_process);

			// Se ci sono eventi, inserimento del processo nella lista dei processi
			if (num_events) {
				Process* new_process_ptr = (Process*)malloc(sizeof(Process));
				*new_process_ptr = new_process;
				if(List_find_process(&os.processes, (ListItem*)new_process_ptr)){
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
		if(!processes_ok || processes_not_ok){
			printf("\n");
			if(!processes_ok) print_message_e(9);
			if(processes_not_ok) {
				printEscape("48;5;234"); printf("%d processes have failed loading. ", processes_not_ok); printEscape("0");
			}
			print_message_e(10);
			fflush(stdout);
			int yn;
			while(1){
				changemode(1);
				while(!kbhit()){}
				yn = getchar();
				changemode(0);

				if(yn=='y') goto insert_filename;
				else if(yn=='n'){
					if(! os.processes.size){
						printf("\n");
						printPidList_AUX(&os.processes, "processes", -1);
						printf("There are no processes.\n\n");
						print_message_e(5);
						return EXIT_SUCCESS;
					}
					break;
				}
				else{
					print_message_e(4);
					continue;
				}
			}
		}
		printf("\n");
		printPidList_AUX(&os.processes, "processes", -1);

		// Nuovi inserimenti
		print_message_e(11);
		fflush(stdout);
		int yn;
		while(1){
			changemode(1);
			while(!kbhit()){}
			yn = getchar();
			changemode(0);

			if(yn=='y') goto insert_filename;
			else if(yn=='n'){
				break;
			}
			else{
				print_message_e(4);
				continue;
			}
		}

		// Inserimento numero di core
		int core = gets_core();
		srr_args.core = core;
		printf("\nNumber of CPUs: %d\n", core);

		// START
		char run = 1;
		int steps;
		while(run) {
			steps = gets_steps();
			if(steps==-1) break;
			else if(steps==0){
				while(os.running.first || os.ready.first || os.waiting.first || os.processes.first) OS_simStep(&os);
				run = 0;
				break;
			}
			else {
				for(int i=0; i<steps; i++){
					if(!(os.running.first || os.ready.first || os.waiting.first || os.processes.first)){
						run = 0;
						break;
					}
					OS_simStep(&os);
				}
			}
		}
		// STOP
	}

	else if(loading_mode=='i'){
		print_message_e(7);
	}





	print_message_e(5);
	return EXIT_SUCCESS;
	// *** inserire controlli numero di argomenti

	int core = atoi(argv[1]);
	
	OS_init(&os);
	scheduler_args srr_args;
	srr_args.core = core;
	os.schedule_args = &srr_args;
	os.schedule_fn = schedulerSJF;
	
	

	for (int i=2; i<argc; ++i){
		Process new_process;
		int num_events = Process_load_file(&new_process, argv[i]);
		Process_CalculatePrediction(&new_process);

		printf("loading [%s], pid: %d, events:%d", argv[i], new_process.pid, num_events);
		if (num_events) {
			Process* new_process_ptr = (Process*)malloc(sizeof(Process));
			*new_process_ptr = new_process;
			List_pushBack(&os.processes, (ListItem*)new_process_ptr); // inserisce alla fine
		}
	}
	printf("num processes in queue %d\n", os.processes.size);
	while(os.running.first || os.ready.first || os.waiting.first || os.processes.first) {
		OS_simStep(&os);
	}
}
