#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "os.h"
#include "aux.h"
#include "scheduler_SJF.h"

#define MAX_SIZE_STRING 1024

char* message_welcome = "\e[1;7mWelcome to CPU Scheduler Simulator: Preemptive scheduler with Shortest Job First\e[0m\n\n";
char* message_press_1 = "\e[48;5;234mPress \e[1;3mf\e[22;23m for loading processes from file or \e[1;3mi\e[22;23m for loading processes in line (\e[1;3mh\e[22;23m for help, \e[1;3mq\e[22;23m for quit)\e[0m\n\n";
char* message_help_1 = "\e[4mLoading from file:\e[24m the simulator will load the processes written in txt file in \"processes\" folder. Txt file should be like:\n\n\tPROCESS\t\t[id] [arrival time]\n\tCPU_BURST\t[duration]\n\tIO_BURST\t[duration]\n\t...\t\t...\n\nFile must terminate with a IO_BURST line.\n\n\e[4mLoading in line:\e[24m the simulator will load the processes when you ask for it.\n\n";
char* message_invalid = "Invalid button, please try again\n";

char* message_goodbye = "\e[1;7mThank you for using CPU Scheduler Simulator!\e[0m\n";


OS os;

int main(int argc, char** argv) {
	printf("%s", message_welcome);
	printf("%s", message_press_1);

	int loading_mode;
	while(1){
		changemode(1);
		while(!kbhit()){}
		loading_mode = getchar();
		changemode(0);

		if(loading_mode=='f' || loading_mode=='i') break;
		else if(loading_mode=='h'){
			printf("%s", message_help_1);
			printf("%s", message_press_1);	
			continue;
		}
		else if(loading_mode=='q'){
			printf("\n%s\n", message_goodbye);
			return EXIT_SUCCESS;
		}
		else{
			printf("%s", message_invalid);
			continue;
		}
	}


	if(loading_mode=='f'){
		printf("\e[1;7mLOADING FROM FILE\e[0m\n");
		
		OS_init(&os);
		scheduler_args srr_args;
		os.schedule_args = &srr_args;
		os.schedule_fn = schedulerSJF;

		insert_filename: ;
		char* files = (char*) malloc(MAX_SIZE_STRING);
		do{
			printf("\e[48;5;234mInsert txt files contained in \e[3mprocesses\e[23m folder (ex: p1 p2 p3):\e[0m ");
			fgets(files, MAX_SIZE_STRING, stdin);
		} while( !strcmp(files, "\n") );
		printf("\n");
		
		int processes_ok = 0;
		int processes_not_ok = 0;

		char* token = strtok(files, " \n");
		while(token){
			// printf("token:\t%s\t", token);
			char* copy = (char*) malloc(strlen(token)+1);
			strcpy(copy, token);
			char* path = "processes/";
			char* extension = ".txt";
			char* filename = (char*) malloc(strlen(token)+strlen(path)+strlen(extension)+1);
			strcpy(filename, path);
			strcat(filename, copy);
			strcat(filename, extension);
			// printf("\tfilename:\t%s\n", filename);
			free(copy);
			
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
			Process_CalculatePrediction(&new_process);

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
		if(!processes_ok || processes_not_ok){
			printf("\n");
			if(!processes_ok) printf("\e[48;5;234mNo process has ben loaded. \e[0m");
			if(processes_not_ok) printf("\e[48;5;234m%d processes have failed loading. \e[0m", processes_not_ok);
			printf("\e[48;5;234mDo you want to try again? (\e[1;3my\e[22;23m for yes, \e[1;3mn\e[22;23m for no)\e[0m\n");
			int yn;
			while(1){
				changemode(1);
				while(!kbhit()){}
				yn = getchar();
				changemode(0);

				if(yn=='y') goto insert_filename;
				else if(yn=='n'){
					if(! &os.processes.size){
						printf("\n%s\n", message_goodbye);
						return EXIT_SUCCESS;
					}
					break;
				}
				else{
					printf("%s", message_invalid);
					continue;
				}
			}
		}
		printf("\n");
		printPidList_AUX(&os.processes, "processes", -1);
		printf("\n");

		printf("\e[48;5;234mDo you want to load more processes? (\e[1;3my\e[22;23m for yes, \e[1;3mn\e[22;23m for no)\e[0m\n");
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
				printf("%s", message_invalid);
				continue;
			}
		}

		int core = gets_core(16, "Insert number of available CPUs:");
		srr_args.core = core;
		printf("\nNumber of CPUs: %d\n", core);
		
		// printf("num processes in queue %d\n", os.processes.size);

		char run = 1;
		int steps;
		while(run) {
			steps = gets_steps(16, "How many steps do you want to go forward? (\e[1;3m0\e[22;23m or \e[1;3mall\e[22;23m for skip to end, \e[1;3mENTER\e[22;23m for one step, \e[1;3mq\e[22;23m for quit)");
			// printf("\nsteps= %d", steps);
			if(steps==-1) break;
			else if(steps==0){
				// printf("\nsteps= %d", steps);
				while(os.running.first || os.ready.first || os.waiting.first || os.processes.first) OS_simStep(&os);
				run = 0;
				break;
			}
			else {
				for(int i=0; i<steps; i++){
					// printf("\nsteps= %d", steps);

					if(!(os.running.first || os.ready.first || os.waiting.first || os.processes.first)){
						run = 0;
						break;
					}
					OS_simStep(&os);
				}
			}
		}
	}

	else if(loading_mode=='i'){
		printf("IN LINE\n");
	}





	printf("\n%s\n", message_goodbye);
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
