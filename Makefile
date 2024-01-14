CC=gcc
CCOPTS=--std=gnu99 -Wall
CCDEBUG= -D_LIST_DEBUG_ #-D_DEBUG #-D_DEBUG_SCHEDULER

OBJS=linked_list.o\
	 process.o\
     os.o

HEADERS=linked_list.h  process.h

BINS= sched_sim

.phony: clean all


all:	$(BINS)

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) $(CCDEBUG) -c -o $@  $<

sched_sim:	sched_sim.c $(OBJS)
	$(CC) $(CCOPTS) $(CCDEBUG) -o $@ $^ -lm

clean:
	rm -rf *.o *~ $(OBJS) $(BINS)
