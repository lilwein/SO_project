CC=gcc
CCOPTS=--std=gnu99 -Wall
CCDEBUG= -D_LIST_DEBUG_ #-D_DEBUG #-D_DEBUG_SCHEDULER
CCANSI= #-D_NO_ANSI

OBJS=linked_list.o\
	 process.o\
     os.o\
	 aux.o\
	 scheduler_SJF.o\
	 scheduler_RR.o

HEADERS=linked_list.h process.h os.h aux.h scheduler.h

BINS= simulator

.phony: clean all


all:	$(BINS)

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) $(CCDEBUG) $(CCANSI) -c -o $@  $<

simulator:	simulator.c $(OBJS)
	$(CC) $(CCOPTS) $(CCDEBUG) $(CCANSI) -o $@ $^ -lm

clean:
	rm -rf *.o *~ $(OBJS) $(BINS)
