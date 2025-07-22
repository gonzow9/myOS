#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pcb.h"
#include "thread_scheduler.h"

// Simple test program to demonstrate multi-threading
int main() {
    printf("Multi-threading Test Program\n");
    printf("============================\n\n");
    
    // Create a simple process
    FILE *test_file = fopen("test_script.txt", "w");
    if (test_file) {
        fprintf(test_file, "echo 'Hello from thread 1'\n");
        fprintf(test_file, "echo 'Hello from thread 2'\n");
        fprintf(test_file, "echo 'Hello from thread 3'\n");
        fprintf(test_file, "echo 'Hello from thread 4'\n");
        fclose(test_file);
    }
    
    // Create process from file
    struct PCB *pcb = create_process("test_script.txt");
    if (!pcb) {
        printf("Failed to create process\n");
        return 1;
    }
    
    printf("Process created with PID: %zu\n", pcb->pid);
    printf("Number of instructions: %zu\n", pcb->line_count);
    
    // Run process with multi-threading
    printf("\nRunning process with 4 threads...\n");
    run_process_multithreaded(pcb, 4);
    
    // Clean up
    free_pcb(pcb);
    unlink("test_script.txt");
    
    printf("\nMulti-threading test completed!\n");
    return 0;
} 