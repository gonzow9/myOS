#include "pcb.h"

// Forward declaration of thread queue
struct thread_queue;

struct thread_scheduler {
    struct thread_queue *ready_queue; // Queue of ready threads
    struct thread_queue *blocked_queue; // Queue of blocked threads
    struct TCB *running_thread; // Currently running thread
    pthread_mutex_t scheduler_mutex; // Mutex for scheduler operations
    int max_threads; // Maximum number of threads allowed
    int current_threads; // Current number of threads
};

// Thread scheduler functions
struct thread_scheduler *create_thread_scheduler(int max_threads);
void free_thread_scheduler(struct thread_scheduler *scheduler);
int add_thread_to_scheduler(struct thread_scheduler *scheduler, struct TCB *thread);
struct TCB *get_next_thread(struct thread_scheduler *scheduler);
void block_thread(struct thread_scheduler *scheduler, struct TCB *thread);
void unblock_thread(struct thread_scheduler *scheduler, struct TCB *thread);
void terminate_thread(struct thread_scheduler *scheduler, struct TCB *thread);
int scheduler_has_work(struct thread_scheduler *scheduler);

// Thread execution functions
struct TCB *run_thread_to_completion(struct TCB *thread);
struct TCB *run_thread_for_n_steps(struct TCB *thread, size_t n);
void *thread_execution_function(void *arg);

// Multi-threaded process execution
struct PCB *run_process_multithreaded(struct PCB *pcb, int num_threads); 