#include "process.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	if (argc<3){
		printf("usage %s <in> <out>\n", argv[0]);
		exit(-1);
	}
	Process p;
	int num_events = Process_load(&p,argv[1]);
	printf("read [%s], num events: %d\n",argv[1], num_events);
	num_events = Process_save(&p,argv[2]);
	printf("saved [%s], num events: %d\n",argv[2], num_events);
	return 0;
}
