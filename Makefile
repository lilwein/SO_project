CC=gcc
CCOPTS=--std=gnu99 -Wall -D_LIST_DEBUG_ 
AR=ar

OBJS=linked_list.o\
	 process.o\
     os.o

HEADERS=linked_list.h  process.h

BINS= sched_sim

#disastros_test

.phony: clean all


all:	$(BINS)

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

sched_sim:	sched_sim.c $(OBJS)
	$(CC) $(CCOPTS) -o $@ $^

clean:
	rm -rf *.o *~ $(OBJS) $(BINS)
