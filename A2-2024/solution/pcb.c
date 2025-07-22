#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset
#include "shell.h" // MAX_USER_INPUT
#include "shellmemory.h"
#include "pcb.h"

int pcb_has_next_instruction(struct PCB *pcb) {
    // have next if pc < line_count.
    // Sanity check: count = 0  ==> never have next. Good!
    return pcb->pc < pcb->line_count;
}

size_t pcb_next_instruction(struct PCB *pcb) {
    size_t i = pcb->line_base + pcb->pc;
    pcb->pc++;
    return i;
}

struct PCB *create_process(const char *filename) {
    // We have 2 main tasks:
    // load all the code in the script file into shellmemory, and
    // allocate+fill a PCB.

    // We don't want to allocate a PCB until we know we actually need one,
    // so let's first make sure we can open the file.
    FILE *script = fopen(filename, "rt");
    if (!script) {
        perror("failed to open file for create_process");
        return NULL;
    }
    struct PCB *pcb = create_process_from_FILE(script);
    // Update the pcb name according to the filename we received.
    pcb->name = strdup(filename);
    return pcb;
}

struct PCB *create_process_from_FILE(FILE *script) {
    // We can open the file, so we'll be making a process.
    struct PCB *pcb = malloc(sizeof(struct PCB));

    // The PID is the only weird part. They need to be distinct,
    // so let's use a static counter.
    static pid fresh_pid = 1;
    pcb->pid = fresh_pid++;

    // name should be the empty string, according to doc comment.
    pcb->name = "";
    // next should be NULL, according to doc comment.
    pcb->next = NULL;

    // pc is always initially 0.
    pcb->pc = 0;

    // Initialise thread support
    pcb->thread_count = 0;
    pcb->threads = NULL;
    pthread_mutex_init(&pcb->process_mutex, NULL);

    // create initial values for base and count, in case we fail to read
    // any lines from the file. That way we'll end up with an empty process
    // that terminates as soon as it is scheduled -- reasonable behavior for
    // an empty script.
    // If we don't do this, and the loop below never runs, base would be
    // unitialized, which would be catastrophic.
    pcb->line_count = 0;
    pcb->line_base = 0;

    // We're told to assume lines of files are limited to 100 characters.
    // That's all well and good, but for implementing # we need to read
    // actual user input, and _that_ is limited to 1000 characters.
    // It's unclear if we should assume it's also limited to 100 for this
    // purpose. If you did assume that, that's OK! We didn't.
    char linebuf[MAX_USER_INPUT];
    while (!feof(script)) {
        memset(linebuf, 0, sizeof(linebuf));
        fgets(linebuf, MAX_USER_INPUT, script);

        size_t index = allocate_line(linebuf);
        // If we've run out of memory, clean up the partially-allocated
        // pcb and return NULL.
        if (index == (size_t)(-1)) {
            free_pcb(pcb);
            fclose(script);
            return NULL;
        }

        if (pcb->line_count == 0) {
            // do this on the first iteration only.
            pcb->line_base = index;
        }
        pcb->line_count++;
    }

    // We're done with the file, don't forget to close it!
    fclose(script);

    // duration should initially match line_count.
    pcb->duration = pcb->line_count;

    return pcb;
}

void free_pcb(struct PCB *pcb) {
    // Free all threads first
    struct TCB *current_thread = pcb->threads;
    while (current_thread) {
        struct TCB *next_thread = current_thread->next;
        free_thread(current_thread);
        current_thread = next_thread;
    }
    
    // Destroy the mutex
    pthread_mutex_destroy(&pcb->process_mutex);
    
    for (size_t ix = pcb->line_base; ix < pcb->line_base + pcb->line_count; ++ix) {
        free_line(ix);
    }
    // Free the process name, but only if it's not the empty string.
    // The empty name (for the shell input process) was not malloc'd.
    if (strcmp("", pcb->name)) {
        free(pcb->name);
    }
    free(pcb);
}

// Thread management functions
struct TCB *create_thread(struct PCB *parent) {
    struct TCB *thread = malloc(sizeof(struct TCB));
    if (!thread) return NULL;
    
    static tid fresh_tid = 1;
    thread->tid = fresh_tid++;
    thread->parent_pcb = parent;
    thread->pc = 0; // Start from beginning
    thread->stack_base = 0; // Will be allocated when needed
    thread->stack_size = 0;
    thread->state = 0; // Ready state
    thread->next = NULL;
    
    return thread;
}

void free_thread(struct TCB *thread) {
    if (thread) {
        free(thread);
    }
}

int tcb_has_next_instruction(struct TCB *tcb) {
    return tcb->pc < tcb->parent_pcb->line_count;
}

size_t tcb_next_instruction(struct TCB *tcb) {
    size_t i = tcb->parent_pcb->line_base + tcb->pc;
    tcb->pc++;
    return i;
}

void add_thread_to_process(struct PCB *pcb, struct TCB *thread) {
    pthread_mutex_lock(&pcb->process_mutex);
    
    // Add to the beginning of the thread list
    thread->next = pcb->threads;
    pcb->threads = thread;
    pcb->thread_count++;
    
    pthread_mutex_unlock(&pcb->process_mutex);
}

void remove_thread_from_process(struct PCB *pcb, struct TCB *thread) {
    pthread_mutex_lock(&pcb->process_mutex);
    
    if (pcb->threads == thread) {
        pcb->threads = thread->next;
    } else {
        struct TCB *current = pcb->threads;
        while (current && current->next != thread) {
            current = current->next;
        }
        if (current) {
            current->next = thread->next;
        }
    }
    pcb->thread_count--;
    
    pthread_mutex_unlock(&pcb->process_mutex);
}
