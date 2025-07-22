#pragma once
#include <stddef.h>
#include <stdio.h>
#include <pthread.h> 

typedef size_t pid;
typedef size_t tid; // Thread ID

// Thread Control Block structure
struct TCB {
    tid tid;
    struct PCB *parent_pcb; // Parent process
    size_t pc; // Program counter for this thread
    size_t stack_base; // Stack base address
    size_t stack_size; // Stack size
    int state; // Thread state: 0=ready, 1=running, 2=blocked, 3=terminated
    struct TCB *next; // For queue management
};

// A process info struct.
struct PCB {
    pid pid;
    // Store the process name, which we can use to detect that multiple
    // processes with the same name are being scheduled.
    // We set it to the empty string if the process name is not known,
    // which should only happen when the process is the 'shell input'
    // background process.
    // Note: on Linux, knowing the filenames is not enough to guarantee
    // that the files are actually different.
    // Reasons for this, e.g. `a/b` is the same file as
    // `a/../a/b` but also so-called hardlinks. There's a way to detect
    // if two open files have the same inode number, which is what we
    // would want to do in a real OS.
    // But for this purposes, just going to stick to doing comparison by name.
    char *name;
    // Base+count form a base-bounds pair for the memory allocated
    // to this process. Seeing it this way helps see how we can replace
    // it with pseudo-paging
    size_t line_base;
    size_t line_count;

    // This field is used for SJF and aging, and should initially have
    // the same value as line_count.
    size_t duration;

    // pc is the number of the instruction next to execute.
    // For example, it is initially 0, regardless of the value of
    // line_base. (like the "virtual address" of the next insn.)
    size_t pc;

    // Thread support
    int thread_count; // Number of threads in this process
    struct TCB *threads; // Linked list of threads
    pthread_mutex_t process_mutex; // Mutex for process-level synchronisation

    // The only purpose here of PCBs is to manage
    // scheduling; the multiprocessing structure simply isn't complicated
    // enough to warrant a factored-out PCB struct that can be used
    // in many different contexts. So we entangle it with the queue by just
    // including the next pointer here directly. The alternative would be
    // to invent a queue structure that has a pointer to a PCB,
    // and manage it separately.
    // If this PCB is the tail of the queue, next is NULL.
    struct PCB *next;
};

// Returns non-zero iff there are more instructions to execute.
int pcb_has_next_instruction(struct PCB *pcb);
// Get the shellmemory index of the next instruction, and increment pc.
size_t pcb_next_instruction(struct PCB *pcb);
// Create a new process from the given filename:
//   1. Allocates a new PCB
//   2. Loads the code from the script file into shellmemory
//   3. Does NOT enqueue the PCB to any scheduling queue (next is NULL)
struct PCB *create_process(const char *filename);
// Like create_process, but takes a FILE* directly.
// Ownership of the FILE* is taken and it will be closed.
struct PCB *create_process_from_FILE(FILE *f);
// Cleanup a process:
//   1. Free all shellmemory used by the process code
//   2. Free the PCB
void free_pcb(struct PCB *pcb);

// Thread management functions
struct TCB *create_thread(struct PCB *parent);
void free_thread(struct TCB *thread);
int tcb_has_next_instruction(struct TCB *tcb);
size_t tcb_next_instruction(struct TCB *tcb);
void add_thread_to_process(struct PCB *pcb, struct TCB *thread);
void remove_thread_from_process(struct PCB *pcb, struct TCB *thread);


