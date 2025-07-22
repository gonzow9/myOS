CC=gcc
#CFLAGS=-g -O0 #-DNDEBUG
CFLAGS=-DNDEBUG

mysh: shell.c interpreter.c shellmemory.c
	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c pcb.c queue.c schedule_policy.c thread_scheduler.c
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o pcb.o queue.o schedule_policy.o thread_scheduler.o -lpthread

test_thread: test_thread.c
	$(CC) $(CFLAGS) -c test_thread.c pcb.c thread_scheduler.c queue.c shellmemory.c
	$(CC) $(CFLAGS) -o test_thread test_thread.o pcb.o thread_scheduler.o queue.o shellmemory.o -lpthread

clean: 
	rm mysh test_thread; rm *.o
