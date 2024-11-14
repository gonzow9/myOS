CC=gcc

# Set default values for framesize and varmemsize if not provided
framesize ?= 30     # Default frame store size
varmemsize ?= 100   # Default variable store size

# Compiler flags, including the macros for framesize and varmemsize
CFLAGS = -g -D FRAME_STORE_SIZE=$(framesize) -D VARIABLE_STORE_SIZE=$(varmemsize)

mysh: shell.c interpreter.c shellmemory.c
	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o

clean: 
	rm mysh; rm *.o
