#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <dirent.h>
#include "shellmemory.h"
#include "shell.h"
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_LINE_LENGTH 100       // Maximum length of each line of code
#define MAX_LINES 1000            // Maximum number of lines in a script
#define MAX_TOKENS 5
#define MAX_MEMORY 1000   // Maximum number of lines that the shell can store
#define MAX_CHAR_INPUT 100  // Maximum characters per line of user input
#define MAX_ARGS_SIZE 7
#define FRAME_SIZE 3              // Each frame holds 3 lines
#define NUM_FRAMES (FRAME_STORE_SIZE / FRAME_SIZE)

typedef struct LoadedScript {
    char *script_name;             // Name of the script
    char *backing_store_filename;  // Unique filename in the backing store
    int ref_count;                 // Number of PCBs using this script
    int pages_max;                 // Total number of pages in the script
    int *pageTable;                // Page table mapping page numbers to frame numbers
    struct LoadedScript *next;     // Pointer to the next LoadedScript in the list
    int script_length;             // Exact number of lines in the script
} LoadedScript;

typedef struct PCB {
    int pid;                     // Unique Process ID
    int PC_page;                 // Current page number in execution
    int PC_offset;               // Offset within the current page
    int pages_max;               // Total number of pages in the script
    int *pageTable;              // Page table mapping page numbers to frame numbers
    int job_length_score;        // Job length score for aging
    LoadedScript *loaded_script; // Pointer to the LoadedScript
    struct PCB *next;            // Pointer to the next PCB (for the ready queue)
    //int pc;                      // Program Counter
} PCB;

PCB *ready_queue = NULL; // A global ready queue for the PCBs
int pid_counter = 0;
LoadedScript *loaded_scripts = NULL; // Head of the loaded scripts list
char frame_store[NUM_FRAMES][FRAME_SIZE][MAX_LINE_LENGTH];
int frame_occupied[NUM_FRAMES]; // 0 = free, 1 = occupied
int global_time = 0;
int frame_last_used[NUM_FRAMES];

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
int echo(char *str);
int custom_sort(const struct dirent **a, const struct dirent **b);
int my_ls();
int my_mkdir(char *dirname);
int is_alphanumeric(const char *str);
int my_touch(char *filename);
int my_cd(char *dirname);
int contains_alphanum(const char *str);
void scheduler();
int exec(char* command_args[], int args_size);
void sort_ready_queue_sjf();
void scheduler_sjf();
void scheduler_rr();
void scheduler_sjf_aging();
void age_jobs();
void sort_ready_queue_by_score();

// Memory managment
void init_backing_store();
LoadedScript *find_loaded_script(char *script_name);
void add_loaded_script(LoadedScript *script);
void remove_loaded_script(LoadedScript *script);
void init_frame_store(); 
int find_free_frame();
void load_page_into_frame(char page_lines[FRAME_SIZE][MAX_LINE_LENGTH], int frame_number);
void copy_script_to_backing_store(LoadedScript *loaded_script);
void add_to_ready_queue(PCB *pcb);
int load_script_initial_pages(LoadedScript *loaded_script);
char* generate_backing_store_filename(const char* script_name);
void handle_page_fault(PCB *pcb);
int evict_random_frame();
void update_page_tables(int frame_number);
int load_page_into_memory(LoadedScript *loaded_script, int page_number, int frame_number);
int evict_lru_frame();

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
    // Store the value (using your function to set the variable)
    mem_set_value(var, value);
    return 0;
}

int print(char *var) {
    printf("%s\n", mem_get_value(var)); 
    return 0;
}

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
    // Initialize backing store if not already initialized
    init_backing_store();

    // Check if script is already loaded
    LoadedScript *loaded_script = find_loaded_script(script);

    if (loaded_script == NULL) {
        // Create LoadedScript and set ref_count to 1
        loaded_script = (LoadedScript *)malloc(sizeof(LoadedScript));
        loaded_script->script_name = strdup(script);
        loaded_script->ref_count = 1;
        loaded_script->next = NULL;

        // Copy script to backing store and initialize pages_max, pageTable, 
        // and script_length of the loaded_script
        copy_script_to_backing_store(loaded_script);

        // Load the first two pages into frames
        int success = load_script_initial_pages(loaded_script);
        if (success != 0) {
            printf("Error: Could not load initial pages of script %s\n", script);
            free(loaded_script->script_name);
            free(loaded_script);
            return -1;
        }
        add_loaded_script(loaded_script);
    } else {
        // Script is already loaded
        // Increment reference count
        loaded_script->ref_count++;
    }

    // Create PCB for the program
    PCB *pcb = (PCB *)malloc(sizeof(PCB));
    pcb->pid = ++pid_counter;
    pcb->PC_page = 0;
    pcb->PC_offset = 0;
    pcb->loaded_script = loaded_script;
    pcb->next = NULL;
    // Initialize job_length_score based on the actual number of instructions
    pcb->job_length_score = loaded_script->script_length;

    // Add the PCB to the ready queue
    add_to_ready_queue(pcb);

    // Call the scheduler to run the processes in the ready queue
    scheduler_rr();

    return 0;  
}

void scheduler() {
    while (ready_queue != NULL) {
        PCB *pcb = ready_queue;  // Get the process at the head of the queue
        ready_queue = ready_queue->next;  // Move to the next process

        // Execute instructions
        while (pcb->PC_page < pcb->pages_max) {
            int frame_number = pcb->pageTable[pcb->PC_page];

            if (frame_number == -1) {
                printf("Error: Page %d not loaded in memory\n", pcb->PC_page);
                // Handle error (e.g., terminate process)
                break;
            }

            char *instruction = frame_store[frame_number][pcb->PC_offset];

            // Execute instruction
            parseInput(instruction);

            // Update PC_offset and PC_page
            pcb->PC_offset++;
            if (pcb->PC_offset >= FRAME_SIZE) {
                pcb->PC_offset = 0;
                pcb->PC_page++;
            }
        }

        // After execution, clean up

        // Decrement the ref_count of the LoadedScript
        pcb->loaded_script->ref_count--;

        // Check if ref_count is zero
        if (pcb->loaded_script->ref_count == 0) {
            // Free the LoadedScript and remove it from the list
            remove_loaded_script(pcb->loaded_script);
        }

        // Free the PCB
        free(pcb->pageTable);
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
            // doesn't contain an alphanumeric char
            return 0; 
        }
    }

    // Contains alphanumeric
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
    // Open the file with "w" mode to create the file if it doesn't exist
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("my_touch: Error creating file");
        return -1;
    }

    // Close the file immediately since we're just creating it
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
    // Initialize backing store in case not already
    init_backing_store();

    // Validate the last argument is a valid policy
    char *policy = args[args_size - 1];
    if (strcmp(policy, "FCFS") != 0 && strcmp(policy, "SJF") != 0 &&
        strcmp(policy, "RR") != 0 && strcmp(policy, "AGING") != 0) {
        printf("Error: Invalid scheduling policy\n");
        return 1;
    }

    // Load up to 3 programs and create PCBs
    int num_programs = args_size - 2;  // Minus "exec" and "policy"

    // Array to store PCBs for the programs
    PCB *pcb_list[3];

    for (int i = 1; i <= num_programs; i++) {
        char *script_name = args[i];
        LoadedScript *loaded_script = find_loaded_script(script_name);

        if (loaded_script == NULL) {
            // Create LoadedScript and set ref_count to 1
            loaded_script = (LoadedScript *)malloc(sizeof(LoadedScript));
            loaded_script->script_name = strdup(script_name);
            loaded_script->ref_count = 1;
            loaded_script->next = NULL;

            // Copy script to backing store and initialize pages_max, pageTable, 
            // and script_length of the loaded_script
            copy_script_to_backing_store(loaded_script);

            // Load the first two pages into frames
            int success = load_script_initial_pages(loaded_script);
            if (success != 0) {
                printf("Error: Could not load initial pages of script %s\n", script_name);
                free(loaded_script->script_name);
                free(loaded_script);
                return -1;
            }

            add_loaded_script(loaded_script);
        } else {
            // Script is already loaded
            // Increment reference count
            loaded_script->ref_count++;
        }

        // Create PCB for the program
        PCB *pcb = (PCB *)malloc(sizeof(PCB));
        pcb->pid = ++pid_counter;
        pcb->PC_page = 0;
        pcb->PC_offset = 0;
        pcb->loaded_script = loaded_script;
        pcb->next = NULL;
        // Initialize job_length_score based on the actual number of instructions
        pcb->job_length_score = loaded_script->script_length;

        // Add the PCB to the list for scheduling
        pcb_list[i - 1] = pcb;
    }

    // Add all PCBs to the ready queue
    for (int i = 0; i < num_programs; i++) {
        add_to_ready_queue(pcb_list[i]);
    }

    // Determine which scheduling policy to use
    if (strcmp(policy, "FCFS") == 0) {
        scheduler();  // FCFS scheduler
    } else if (strcmp(policy, "SJF") == 0) {
        scheduler_sjf();  // SJF scheduler
    } else if (strcmp(policy, "RR") == 0) {
        scheduler_rr();  // RR scheduler with time slice of 2 lines
    } else if (strcmp(policy, "AGING") == 0) {
        scheduler_sjf_aging();  // SJF with aging scheduler
    } else {
        printf("Error: Invalid scheduling policy\n");
        return 1;
    }

    return 0;
}

// Helper function to sort PCBs by job length score (Shortest Job First)
void sort_ready_queue_sjf() {
    if (ready_queue == NULL || ready_queue->next == NULL) return;

    PCB *current, *next;
    int swapped;
    do {
        swapped = 0;
        current = ready_queue;

        while (current->next != NULL) {
            next = current->next;
            if (current->job_length_score > next->job_length_score) {
                // Swap PCBs
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
    sort_ready_queue_sjf();  // Sort by job length score

    while (ready_queue != NULL) {
        PCB *pcb = ready_queue;  // Get the process at the head of the queue
        ready_queue = ready_queue->next;

        // Execute instructions
        while (pcb->PC_page < pcb->pages_max) {
            int frame_number = pcb->pageTable[pcb->PC_page];

            if (frame_number == -1) {
                printf("Error: Page %d not loaded in memory\n", pcb->PC_page);
                // Handle error
                break;
            }

            char *instruction = frame_store[frame_number][pcb->PC_offset];

            // Execute instruction
            parseInput(instruction);

            // Update PC_offset and PC_page
            pcb->PC_offset++;
            if (pcb->PC_offset >= FRAME_SIZE) {
                pcb->PC_offset = 0;
                pcb->PC_page++;
            }
        }

        // After execution, clean up
        pcb->loaded_script->ref_count--;
        if (pcb->loaded_script->ref_count == 0) {
            remove_loaded_script(pcb->loaded_script);
        }
        free(pcb->pageTable);
        free(pcb);
    }
}

// RR Scheduler
void scheduler_rr() {
    PCB *current_job = NULL;
    int time_slice = 2; // Time slice of 2 lines

    while (ready_queue != NULL || current_job != NULL) {
        if (current_job == NULL) {
            // Get the next job from the ready queue
            current_job = ready_queue;
            ready_queue = ready_queue->next;
            current_job->next = NULL;  // Detach from ready queue
        }

        int instructions_executed = 0;

        while (instructions_executed < time_slice) {
            // Calculate the current instruction index
            int current_instruction_index = current_job->PC_page * FRAME_SIZE + current_job->PC_offset;

            // Check if the job has finished
            if (current_instruction_index >= current_job->loaded_script->script_length) {
                // Job has finished
                current_job->loaded_script->ref_count--;
                if (current_job->loaded_script->ref_count == 0) {
                    // Do not clean up pages in the frame store
                    // Remove the loaded script which frees the page table
                    remove_loaded_script(current_job->loaded_script);
                    current_job->loaded_script = NULL;
                }
                free(current_job);
                current_job = NULL;
                break;
            }

            // Get frame number from page table
            int frame_number = current_job->loaded_script->pageTable[current_job->PC_page];
            if (frame_number == -1) {
                // Page fault occurred
                // Interrupt current process and place it at the back of the ready queue
                // Handle page fault: Load missing page
                handle_page_fault(current_job);

                // Break out of the time slice loop
                break;
            }

            // Get instruction
            char *instruction = frame_store[frame_number][current_job->PC_offset];

            // Make a copy of the instruction to prevent modification of the frame store
            char instruction_copy[MAX_LINE_LENGTH];
            strcpy(instruction_copy, instruction);

            // Update global time and frame last used time
            global_time++;
            frame_last_used[frame_number] = global_time;

            // Execute instruction
            parseInput(instruction_copy);

            // Update program counter
            current_job->PC_offset++;
            if (current_job->PC_offset >= FRAME_SIZE) {
                // Move to next page
                current_job->PC_offset = 0;
                current_job->PC_page++;
            }

            instructions_executed++; 
        }

        // If the job is still running, add it back to the ready queue
        if (current_job != NULL) {
            add_to_ready_queue(current_job);
            current_job = NULL;
        }
    }
}

void scheduler_sjf_aging() {
    // First sort jobs by score at the start
    sort_ready_queue_by_score();

    // After sorting, the head of the queue has the lowest score at the start
    PCB *current_job = ready_queue;

    while (ready_queue != NULL || current_job != NULL) {
        if (current_job == NULL) {
            if (ready_queue == NULL) break;  // No jobs left to schedule

            // Get the job with the lowest job_length_score
            current_job = ready_queue;
        }

        // printf("Executing PID %d, PC_page %d, PC_offset %d, job_length_score %d\n",
        // current_job->pid, current_job->PC_page, current_job->PC_offset, current_job->job_length_score);

        // Execute one instruction of the current job
        int frame_number = current_job->pageTable[current_job->PC_page];
        if (frame_number == -1) {
            printf("Error: Page %d not loaded in memory\n", current_job->PC_page);
            // Handle error (terminate process ?)
            // Remove current_job from ready_queue
            ready_queue = ready_queue->next;
            free(current_job->pageTable);
            free(current_job);
            current_job = NULL;
            continue;
        }
        char *instruction = frame_store[frame_number][current_job->PC_offset];
        parseInput(instruction);

        // After aging jobs
        // printf("Aged jobs in ready_queue. Current job_length_scores:\n");
        // PCB *temp = ready_queue;
        // printf("Current Job instr: %s\n", instruction);
        // while (temp != NULL) {
        //     printf("PID %d: %d\n", temp->pid, temp->job_length_score);
        //     temp = temp->next;
        // }

        // Update PC_offset and PC_page
        current_job->PC_offset++;
        if (current_job->PC_offset >= FRAME_SIZE) {
            current_job->PC_offset = 0;
            current_job->PC_page++;
        }

        // Check if the job has finished
        if (current_job->PC_page >= current_job->pages_max) {
            // Move to next lowest score job if current is finished
            ready_queue = ready_queue->next;

            // Decrement ref_count and remove script if necessary
            current_job->loaded_script->ref_count--;
            if (current_job->loaded_script->ref_count == 0) {
                remove_loaded_script(current_job->loaded_script);
            }
            free(current_job->pageTable);
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
        }
    }
}

// Decreases the job_length_score of all jobs except the current job
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

void init_backing_store() {
    DIR *dir = opendir("backing_store");
    struct dirent *entry;
    char filepath[256];

    if (dir == NULL) {
        // Directory does not exist so create it
        if (mkdir("backing_store", 0700) != 0) {
            perror("Error creating backing_store directory");
            exit(1);
        }
    } else {
        // Directory exists, remove its contents
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(filepath, sizeof(filepath), "backing_store/%s", entry->d_name);
            if (unlink(filepath) != 0) {
                perror("Error deleting file in backing_store");
            }
        }
        closedir(dir);
    }
}

LoadedScript *find_loaded_script(char *script_name) {
    LoadedScript *current = loaded_scripts;
    while (current != NULL) {
        if (strcmp(current->script_name, script_name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void add_loaded_script(LoadedScript *script) {
    script->next = loaded_scripts;
    loaded_scripts = script;
}

void remove_loaded_script(LoadedScript *script) {
    // Remove the script from the loaded_scripts list
    LoadedScript *current = loaded_scripts;
    LoadedScript *prev = NULL;

    while (current != NULL) {
        if (current == script) {
            if (prev == NULL) {
                // Removing the head of the list
                loaded_scripts = current->next;
            } else {
                prev->next = current->next;
            }
            
            // Delete the backing store file
            remove(current->backing_store_filename);

            // Free associated memory
            free(current->script_name);
            current->script_name = NULL;

            free(current->backing_store_filename);
            current->backing_store_filename = NULL;

            free(current->pageTable);
            current->pageTable = NULL;

            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void init_frame_store() {
    for (int i = 0; i < NUM_FRAMES; i++) {
        frame_occupied[i] = 0;
        frame_last_used[i] = 0; // initialize last used time to 0
        for (int j = 0; j < FRAME_SIZE; j++) {
            frame_store[i][j][0] = '\0'; // Initialize strings to empty
        }
    }
}

int find_free_frame() {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frame_occupied[i] == 0) {
            return i;
        }
    }
    return -1; // No free frames
}

void load_page_into_frame(char page_lines[FRAME_SIZE][MAX_LINE_LENGTH], int frame_number) {
    for (int i = 0; i < FRAME_SIZE; i++) {
        strcpy(frame_store[frame_number][i], page_lines[i]);
    }
    // Not setting occupied!
    frame_occupied[frame_number] = 1;
}

// Copy script data to backing store and initialize page table for loaded script
void copy_script_to_backing_store(LoadedScript *loaded_script) {
    // Generate a unique backing store filename
    char *backing_store_filename = generate_backing_store_filename(loaded_script->script_name);
    loaded_script->backing_store_filename = backing_store_filename;

    // Open the original script and backing store files
    FILE *source = fopen(loaded_script->script_name, "r");
    if (source == NULL) {
        printf("Error: Script %s not found\n", loaded_script->script_name);
        return;
    }

    FILE *dest = fopen(backing_store_filename, "w");
    if (dest == NULL) {
        printf("Error: Could not create backing store file\n");
        fclose(source);
        return;
    }

    // Copy the script to the backing store and count the lines
    char line[MAX_LINE_LENGTH];
    int line_count = 0;
    while (fgets(line, MAX_LINE_LENGTH - 1, source) != NULL) {
        fputs(line, dest);
        line_count++;
    }

    fclose(source);
    fclose(dest);

    // Calculate the total number of pages
    loaded_script->pages_max = (line_count + FRAME_SIZE - 1) / FRAME_SIZE;

    // Set script_length to the exact number of lines
    loaded_script->script_length = line_count;

    // Initialize the page table with -1 (pages not loaded)
    loaded_script->pageTable = malloc(sizeof(int) * loaded_script->pages_max);
    if (loaded_script->pageTable == NULL) {
        printf("Error: Could not allocate memory for page table\n");
        return;
    }
    for (int i = 0; i < loaded_script->pages_max; i++) {
        loaded_script->pageTable[i] = -1;
    }
}

int load_script_initial_pages(LoadedScript *loaded_script) {
    // Open the backing store file
    FILE *file = fopen(loaded_script->backing_store_filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s in backing store\n", loaded_script->backing_store_filename);
        return -1;
    }

    // Load only the first two pages
    int pages_to_load = (loaded_script->pages_max >= 2) ? 2 : 1;
    int line_index = 0;
    char line[MAX_LINE_LENGTH];

    for (int page_number = 0; page_number < pages_to_load; page_number++) {
        char page_lines[FRAME_SIZE][MAX_LINE_LENGTH];

        // Read FRAME_SIZE lines for the page
        for (int offset = 0; offset < FRAME_SIZE; offset++) {
            if (fgets(line, MAX_LINE_LENGTH - 1, file) != NULL) {
                line[strcspn(line, "\r\n")] = '\0';  // remove newline
                strcpy(page_lines[offset], line);
                line_index++;
            } else {
                strcpy(page_lines[offset], "");  // empty string if EOF
            }
        }

        // Find a free frame
        int frame_number = find_free_frame();
        if (frame_number == -1) {
            // Theoretically shouldn't never happen at the start up
            printf("Error: No free frames available\n");
            fclose(file);
            return -1;
        }

        // Load the page into the frame
        load_page_into_frame(page_lines, frame_number);

        // Update the page table
        loaded_script->pageTable[page_number] = frame_number;
    }

    fclose(file);
    return 0;  // Success
}

// Function to generate a unique filename for the backing store
char* generate_backing_store_filename(const char* script_name) {
    static int backing_store_counter = 0;
    char* filename = malloc(256);  // Adjust size as needed

    if (filename == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for backing store filename\n");
        exit(1);
    }

    // Generate a unique filename using the script name and a counter
    snprintf(filename, 256, "backing_store_%s_%d.txt", script_name, backing_store_counter++);
    return filename;
}

void handle_page_fault(PCB *pcb) {
    // Try to find a free frame
    int frame_number = find_free_frame();

    if (frame_number == -1) {
        // No free frames; need to evict a victim frame
        frame_number = evict_lru_frame();

        // Print victim page contents
        printf("Page fault! Victim page contents:\n\n");
        for (int i = 0; i < FRAME_SIZE; i++) {
            printf("%s\n", frame_store[frame_number][i]);
        }
        printf("\nEnd of victim page contents.\n");

        // Update the page tables of the processes owning the victim frame
        update_page_tables(frame_number);
    } else {
        // Frame store is not full
        printf("Page fault!\n");
    }

    // Load the missing page into the frame
    int success = load_page_into_memory(pcb->loaded_script, pcb->PC_page, frame_number);
    if (success != 0) {
        printf("Error: Could not load page %d for PID %d\n", pcb->PC_page, pcb->pid);
        // Handle error (e.g., terminate process)
        pcb->loaded_script->ref_count--;
        if (pcb->loaded_script->ref_count == 0) {
            free(pcb->loaded_script->pageTable);
            remove_loaded_script(pcb->loaded_script);
        }
        free(pcb);
        pcb = NULL;
        return;
    }

    // Update the page table
    pcb->loaded_script->pageTable[pcb->PC_page] = frame_number;
}

int evict_random_frame() {
    int victim_frame = rand() % NUM_FRAMES;

    // Mark the frame as free
    frame_occupied[victim_frame] = 0;

    return victim_frame;
}

void update_page_tables(int frame_number) {
    // Iterate over all loaded scripts and their page tables
    LoadedScript *current_script = loaded_scripts;  // head pointer to the list
    while (current_script != NULL) {
        for (int i = 0; i < current_script->pages_max; i++) {
            if (current_script->pageTable[i] == frame_number) {
                // Invalidate the page table entry
                current_script->pageTable[i] = -1;
                break;  // Assuming a frame is only in one page table
            }
        }
        current_script = current_script->next;
    }
}

int load_page_into_memory(LoadedScript *loaded_script, int page_number, int frame_number) {
    // Open the backing store file
    FILE *file = fopen(loaded_script->backing_store_filename, "r");
    if (file == NULL) {
        printf("Error: Could not open backing store file %s\n", loaded_script->backing_store_filename);
        return -1;
    }

    // Seek to the start of the required page
    int seek_lines = page_number * FRAME_SIZE;
    char line[MAX_LINE_LENGTH];
    for (int i = 0; i < seek_lines; i++) {
        if (fgets(line, MAX_LINE_LENGTH - 1, file) == NULL) {
            // Reached EOF before the desired page
            fclose(file);
            return -1;
        }
    }

    // Read the page lines
    char page_lines[FRAME_SIZE][MAX_LINE_LENGTH];
    int total_lines_read = page_number * FRAME_SIZE;
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (total_lines_read < loaded_script->script_length) {
            if (fgets(line, MAX_LINE_LENGTH - 1, file) != NULL) {
                line[strcspn(line, "\r\n")] = '\0';
                strcpy(page_lines[i], line);
            } else {
                strcpy(page_lines[i], ""); // Empty string for padding
            }
            total_lines_read++;
        } else {
            strcpy(page_lines[i], ""); // Empty string for padding
        }
    }
    fclose(file);

    // Load the page into the frame
    load_page_into_frame(page_lines, frame_number);

    // Update global time and frame last used time
    global_time++;
    frame_last_used[frame_number] = global_time;

    // Update frame occupancy
    frame_occupied[frame_number] = 1;

    return 0; // Success
}

int evict_lru_frame() {
    int lru_frame = -1;
    int min_time = global_time + 1; // Set to a value higher than any possible last_used_time

    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frame_occupied[i] == 1 && frame_last_used[i] < min_time) {
            min_time = frame_last_used[i];
            lru_frame = i;
        }
    }

    if (lru_frame == -1) {
        printf("Error: No frames to evict\n");
        exit(1); 
    }

    // Mark the frame as free
    frame_occupied[lru_frame] = 0;

    return lru_frame;
}