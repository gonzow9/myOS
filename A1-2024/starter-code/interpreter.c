#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <dirent.h>
#include "shellmemory.h"
#include "shell.h"
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_TOKENS 5
#define MAX_MEMORY 1000   // Maximum number of lines that the shell can store
#define MAX_CHAR_INPUT 100  // Maximum characters per line of user input
int MAX_ARGS_SIZE = 7;

char shell_memory[MAX_MEMORY][MAX_CHAR_INPUT];

typedef struct PCB {
    int pid;                    // Unique Process ID
    int start_position;         // Start position in shell memory
    int current_instruction;    // Program counter
    int length;                 // Number of instructions in the script
    int job_length_score;       // Job length score for aging
    struct PCB *next;           // Pointer to the next PCB (for the ready queue)
} PCB;


int badcommand(){
    printf("Unknown Command\n");
    return 1;
}

int badset(){
    printf("Bad command: Too many tokens\n");
    return 1;
}

// For run command only
int badcommandFileDoesNotExist(){
    printf("Bad command: File not found\n");
    return 3;
}

int help();
int quit();
int set(char* var, char* value);
int print(char* var);
int run(char* script);
int badcommandFileDoesNotExist();
int echo(char *str);
int custom_sort(const struct dirent **a, const struct dirent **b);
int my_ls();
int my_mkdir(char *dirname);
int is_alphanumeric(const char *str);
int my_touch(char *filename);
int my_cd(char *dirname);
int contains_alphanum(const char *str);\
void scheduler();
int exec(char* command_args[], int args_size);
void sort_ready_queue_sjf();
void scheduler_sjf();
void scheduler_rr();
void scheduler_sjf_aging();
void age_jobs();
void sort_ready_queue_by_score();


// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size) {
    int i;

    if (args_size < 1) {
        return badcommand();
    } else if (args_size > MAX_ARGS_SIZE) {
        return badset();
    }

    for (i = 0; i < args_size; i++) { // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0){
        //help
        if (args_size != 1) return badcommand();
        return help();
    
    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1) return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        // Handle set command with multiple tokens for the value
        if (args_size < 3) return badcommand(); 
        
        // Concatenate tokens starting from command_args[2] up to args_size
        char value[MAX_USER_INPUT * MAX_TOKENS] = "";
        for (i = 2; i < args_size; i++) {
            strcat(value, command_args[i]);
            if (i < args_size - 1) {
                // Add space between tokens
                strcat(value, " ");
            }
        }

        // Call set function with concatenated value
        return set(command_args[1], value);
    
    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2) return badcommand();
        return print(command_args[1]);
    
    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size != 2) return badcommand();
        return run(command_args[1]);
    
    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2) return badcommand();
        return echo(command_args[1]);
    
    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1) return badcommand();
        return my_ls();
    
    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2) return badcommand();
        return my_mkdir(command_args[1]);
    
    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2) return badcommand();
        return my_touch(command_args[1]);
    
    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2) return badcommand();
        return my_cd(command_args[1]);
    
    } else if (strcmp(command_args[0], "exec") == 0) {
        if (args_size < 3 || args_size > 5) {
            return badcommand(); 
        }
        return exec(command_args, args_size);
    
    } else return badcommand();
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    // Creates a copy of the value string to tokenize
    char value_copy[MAX_USER_INPUT];
    strcpy(value_copy, value);
    
    // Tokenize the value string
    char *tokens[MAX_TOKENS + 1]; // One extra to check for too many tokens
    int token_count = 0;
    
    // Store the value (using your function to set the variable)
    mem_set_value(var, value);
    
    return 0;
}

int print(char *var) {
    printf("%s\n", mem_get_value(var)); 
    return 0;
}

// A global ready queue and PCB array for simplicity
PCB *ready_queue = NULL;
int pid_counter = 0;
int memory_position = 0;

// Function to add a PCB to the ready queue
void add_to_ready_queue(PCB *pcb) {
    pcb->next = NULL;  // Important to prevent loops
    if (ready_queue == NULL) {
        ready_queue = pcb;
    } else {
        PCB *current = ready_queue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = pcb;
    }
}


// Modified run function
int run(char *script) {
    FILE *p = fopen(script, "rt");
    if (p == NULL) {
        return badcommandFileDoesNotExist();
    }

    // Create and initialize the PCB for this process
    PCB *pcb = (PCB *)malloc(sizeof(PCB));
    pcb->pid = ++pid_counter;
    pcb->start_position = memory_position;
    pcb->current_instruction = pcb->start_position;
    pcb->next = NULL;

    // Load the script into shell memory
    char line[MAX_CHAR_INPUT];
    int script_length = 0;
    while (fgets(line, MAX_CHAR_INPUT - 1, p) != NULL) {
        if (memory_position >= MAX_MEMORY) {
            printf("Error: Shell memory is full.\n");
            fclose(p);
            free(pcb);
            return -1;
        }
        strcpy(shell_memory[memory_position], line);
        memory_position++;
        script_length++;
    }

    fclose(p);  

    pcb->length = script_length;

    pcb->job_length_score = script_length;

    // Add the PCB to the ready queue
    add_to_ready_queue(pcb);

    // Call the scheduler to run the processes in the ready queue
    scheduler();

    return 0;  
}

// Function to execute processes in the ready queue (FCFS)
void scheduler() {
    while (ready_queue != NULL) {
        PCB *pcb = ready_queue;  // Get the process at the head of the queue
        for (int i = pcb->current_instruction; i < pcb->start_position + pcb->length; i++) {
            parseInput(shell_memory[i]);  // Execute each instruction in the script
        }

        // After execution, remove the process from the queue and clean up
        ready_queue = ready_queue->next;  // Move to the next process
        free(pcb);  // Free memory allocated to the PCB
    }
}

int echo(char *str) {
    if (str[0] != '$') {
        printf("%s\n", str);
    } else {
        // add 1 to the substring pointer after the '$'
        char *var = strchr(str, '$') + 1;
        char *val = mem_get_value(var);
        if (strcmp(val, "Variable does not exist") != 0) {
            printf("%s\n", val);
        } else {
            printf("\n");
        }
    }
    
    return 0;
}

// Custom comparison function for sorting file names
int custom_sort(const struct dirent **a, const struct dirent **b) {
    // Get the file names
    const char *nameA = (*a)->d_name;
    const char *nameB = (*b)->d_name;

    // Sort numbers before letters
    if (isdigit(nameA[0]) && !isdigit(nameB[0])) {
        return -1;
    }
    if (!isdigit(nameA[0]) && isdigit(nameB[0])) {
        return 1;
    }

    // Sort uppercase before lowercase
    if (tolower(nameA[0]) == tolower(nameB[0])) {
        return nameA[0] - nameB[0];
    }

    // Regular alphabetical order
    return strcmp(nameA, nameB);
}

// Filter function to exclude "." and ".."
int filter(const struct dirent *entry) {
    return (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0);
}

int my_ls() {
    struct dirent **namelist;
    int n;

    // Scan current directory and apply filter and custom sort
    n = scandir(".", &namelist, filter, custom_sort);
    if (n < 0) {
        perror("scandir");
        return -1;
    }

    for (int i = 0; i < n; i++) {
        printf("%s\n", namelist[i]->d_name);
        // Free memory allocated by scandir for each pointer entry
        free(namelist[i]);
    }
    free(namelist); 

    return 0;
}

// Helper function to check if a string is alphanumeric
int is_alphanumeric(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalnum(str[i])) {
            // Not alphanumeric
            return 0; 
        }
    }

    // Alphanumeric
    return 1; 
}

// Helper function to check if a string contains an alphanumeric char
int contains_alphanum(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalnum(str[i])) {
            // doesn't contains an alphanumeric char
            return 0; 
        }
    }

    // Alphanumeric
    return 1; 
}


int my_mkdir(char *dirname) {
    int status;

    if (dirname[0] == '$') {
        // Skip the "$" symbol
        char *var = dirname + 1; 
        char *val = mem_get_value(var);
        if (strcmp(val, "Variable does not exist") != 0 && contains_alphanum(val)) {
            status = mkdir(val, 0755);
        } else if (strcmp(val, "Variable does not exist") == 0 || !contains_alphanum(val)) {
            printf("Bad command: my_mkdir\n");
        } else {
            printf("Bad command: my_mkdir\n");
        }
    } else if (is_alphanumeric(dirname)) {
        // just used 0755 permissions
        status = mkdir(dirname, 0755);
    }
    
    return 0;
}

int my_touch(char *filename) {
    // Open the file with O_CREAT flag to create the file if it doesn't exist
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("my_touch: Error creating file");
        return -1;
    }

    // Close the file immediately since we just creating it
    fclose(file);

    return 0;
}

int my_cd(char *dirname) {
    if (!is_alphanumeric(dirname)) {
        printf("Bad command: my_cd\n");
        return 0;
    }

    if (chdir(dirname) == -1) {
        printf("Bad command: my_cd\n");
    }

    return 0;
}

int exec(char* args[], int args_size) {
    // Validate the last argument is a valid policy
    char *policy = args[args_size - 1];
    if (strcmp(policy, "FCFS") != 0 && strcmp(policy, "SJF") != 0 &&
        strcmp(policy, "RR") != 0 && strcmp(policy, "AGING") != 0) {
        printf("Error: Invalid scheduling policy\n");
        return 1;
    }
 
    // Load up to 3 programs and create PCBs
    int num_programs = args_size - 2;  // Minus "exec" and "policy"
    PCB *pcb_list[3];  // Store PCBs for the programs
    for (int i = 1; i <= num_programs; i++) {
        FILE *p = fopen(args[i], "rt");
        if (p == NULL) {
            printf("Error: Could not open file %s\n", args[i]);
            return badcommandFileDoesNotExist();
        }

        // Create PCB for the program
        PCB *pcb = (PCB *)malloc(sizeof(PCB));
        pcb->pid = ++pid_counter;
        pcb->start_position = memory_position;
        pcb->current_instruction = pcb->start_position;
        pcb->next = NULL;

        // Load the program into shell memory
        char line[MAX_CHAR_INPUT];
        int script_length = 0;
        while (fgets(line, MAX_CHAR_INPUT - 1, p) != NULL) {
            if (memory_position >= MAX_MEMORY) {
                printf("Error: Shell memory is full\n");
                fclose(p);
                free(pcb);
                return -1;
            }
            strcpy(shell_memory[memory_position], line);
            memory_position++;
            script_length++;
        }
        fclose(p);

        // Set the length and job length score of the script in the PCB
        pcb->length = script_length;
        pcb->job_length_score = script_length;

        // Add the PCB to the list for scheduling
        pcb_list[i - 1] = pcb;
    }

    // Add all PCBs to the ready queue (same for all policies)
    for (int i = 0; i < num_programs; i++) {
        add_to_ready_queue(pcb_list[i]);
    }

    // Determine which scheduling policy to use
    if (strcmp(policy, "FCFS") == 0) {
        scheduler();  // FCFS scheduler
    } else if (strcmp(policy, "SJF") == 0) {
        scheduler_sjf();  // SJF scheduler
    } else if (strcmp(policy, "RR") == 0) {
        scheduler_rr();  // RR scheduler
    } else if (strcmp(policy, "AGING") == 0) {
        scheduler_sjf_aging();  // SJF with aging scheduler
    } else {
        printf("Error: Invalid scheduling policy\n");
        return 1;
    }

    return 0;
}

// Helper function to sort PCBs by job length (Shortest Job First)
void sort_ready_queue_sjf() {
    if (ready_queue == NULL || ready_queue->next == NULL) return;

    PCB *current, *next;
    int swapped;
    do {
        swapped = 0;
        current = ready_queue;

        while (current->next != NULL) {
            next = current->next;
            if (current->length > next->length) {
                // Swap PCBs if the current one has more lines than the next
                PCB temp = *current;
                *current = *next;
                *next = temp;

                // Restore next pointers after swapping
                PCB *tempNext = current->next;
                current->next = next->next;
                next->next = tempNext;

                swapped = 1;
            }
            current = current->next;
        }
    } while (swapped);
}

// SJF Scheduler
void scheduler_sjf() {
    sort_ready_queue_sjf();  // Sort by job length

    while (ready_queue != NULL) {
        PCB *pcb = ready_queue;  // Get the process at the head of the queue
        for (int i = pcb->current_instruction; i < pcb->start_position + pcb->length; i++) {
            parseInput(shell_memory[i]);  // Execute each instruction in the script
        }

        // After execution, remove the process from the queue and clean up
        ready_queue = ready_queue->next;
        free(pcb);
    }
}

// RR Scheduler
void scheduler_rr() {
    int time_slice = 2;

    while (ready_queue != NULL) {
        PCB *pcb = ready_queue;
        ready_queue = ready_queue->next;  // Remove from front

        // Execute instructions
        for (int i = 0; i < time_slice && pcb->current_instruction < pcb->start_position + pcb->length; i++) {
            parseInput(shell_memory[pcb->current_instruction]);
            pcb->current_instruction++;
        }

        if (pcb->current_instruction >= pcb->start_position + pcb->length) {
            free(pcb);
        } else {
            pcb->next = NULL;  // Prevent potential cycles
            add_to_ready_queue(pcb);
        }
    }
}

void insert_into_ready_queue_sorted(PCB *pcb) {
    if (ready_queue == NULL || pcb->job_length_score < ready_queue->job_length_score) {
        pcb->next = ready_queue;
        ready_queue = pcb;
    } else {
        PCB *current = ready_queue;
        while (current->next != NULL && current->next->job_length_score <= pcb->job_length_score) {
            current = current->next;
        }
        pcb->next = current->next;
        current->next = pcb;
    }
}

// SJF Scheduler with aging policy
void scheduler_sjf_aging() {
    // First sort jobs by score at the start
    sort_ready_queue_by_score();

    // after sorting, the head of queue has lowest score at the start
    PCB *current_job = ready_queue;

    while (ready_queue != NULL || current_job != NULL) {
        if (current_job == NULL) {
            if (ready_queue == NULL) break;  // No jobs left to schedule

            // Get the job with the lowest job_length_score
            current_job = ready_queue;
        }

        // Execute one instruction of the current job
        parseInput(shell_memory[current_job->current_instruction]);
        current_job->current_instruction++;

        // Check if the job has finished
        if (current_job->current_instruction >= current_job->start_position + current_job->length) {
            // move to next lowest score job if current is finished
            ready_queue = ready_queue->next;

            free(current_job);
            current_job = NULL;
            continue;
        }

        // Age the other jobs
        age_jobs();

        // Re-sort the ready queue based on updated job_length_score
        sort_ready_queue_by_score();

        // Check for preemption
        if (ready_queue != NULL && ready_queue->job_length_score < current_job->job_length_score) {
            // Set new current job to be the head of the queue
            current_job = ready_queue;

            //printf("Promoted process %d\n", current_job->pid);
        }
    }
}

// Decreases the job_length_score of all jobs except the head of the queue
void age_jobs() {
    PCB *temp = ready_queue;

    if (temp == NULL) return; // Ready queue is empty

    temp = temp->next; // Skip the head of the ready queue

    while (temp != NULL) {
        if (temp->job_length_score > 0) {
            temp->job_length_score--;
        }
        temp = temp->next;
    }
}


void sort_ready_queue_by_score() {
    if (ready_queue == NULL || ready_queue->next == NULL) return;

    PCB *sorted = NULL;

    while (ready_queue != NULL) {
        PCB *current = ready_queue;
        ready_queue = ready_queue->next;

        // Insert current into sorted list
        if (sorted == NULL || current->job_length_score < sorted->job_length_score) {
            current->next = sorted;
            sorted = current;
        } else {
            PCB *s_current = sorted;
            while (s_current->next != NULL && s_current->next->job_length_score <= current->job_length_score) {
                s_current = s_current->next;
            }
            current->next = s_current->next;
            s_current->next = current;
        }
    }

    ready_queue = sorted;
}