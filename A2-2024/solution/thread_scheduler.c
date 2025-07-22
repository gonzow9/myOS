#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "thread_scheduler.h"
#include "shellmemory.h"
#include "shell.h"

struct thread_queue {
    struct TCB *head;
    struct TCB *tail;
    int size;
};

struct thread_queue *create_thread_queue() {
    struct thread_queue *q = malloc(sizeof(struct thread_queue));
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

void free_thread_queue(struct thread_queue *q) {
    free(q);
}

void enqueue_thread(struct thread_queue *q, struct TCB *thread) {
    thread->next = NULL;
    if (q->tail) {
        q->tail->next = thread;
        q->tail = thread;
    } else {
        q->head = thread;
        q->tail = thread;
    }
    q->size++;
}

struct TCB *dequeue_thread(struct thread_queue *q) {
    if (!q->head) return NULL;
    
    struct TCB *thread = q->head;
    q->head = thread->next;
    if (!q->head) q->tail = NULL;
    q->size--;
    thread->next = NULL;
    return thread;
}

int is_thread_queue_empty(struct thread_queue *q) {
    return q->head == NULL;
}

struct thread_scheduler *create_thread_scheduler(int max_threads) {
    struct thread_scheduler *scheduler = malloc(sizeof(struct thread_scheduler));
    if (!scheduler) return NULL;
    
    scheduler->ready_queue = create_thread_queue();
    scheduler->blocked_queue = create_thread_queue();
    scheduler->running_thread = NULL;
    scheduler->max_threads = max_threads;
    scheduler->current_threads = 0;
    
    pthread_mutex_init(&scheduler->scheduler_mutex, NULL);
    
    return scheduler;
}

void free_thread_scheduler(struct thread_scheduler *scheduler) {
    if (!scheduler) return;
    
    pthread_mutex_destroy(&scheduler->scheduler_mutex);
    free_thread_queue(scheduler->ready_queue);
    free_thread_queue(scheduler->blocked_queue);
    free(scheduler);
}

int add_thread_to_scheduler(struct thread_scheduler *scheduler, struct TCB *thread) {
    pthread_mutex_lock(&scheduler->scheduler_mutex);
    
    if (scheduler->current_threads >= scheduler->max_threads) {
        pthread_mutex_unlock(&scheduler->scheduler_mutex);
        return 0; // Failed to add thread
    }
    
    thread->state = 0; // Ready state
    enqueue_thread(scheduler->ready_queue, thread);
    scheduler->current_threads++;
    
    pthread_mutex_unlock(&scheduler->scheduler_mutex);
    return 1; // Success
}

struct TCB *get_next_thread(struct thread_scheduler *scheduler) {
    pthread_mutex_lock(&scheduler->scheduler_mutex);
    
    struct TCB *thread = dequeue_thread(scheduler->ready_queue);
    if (thread) {
        thread->state = 1; // Running state
        scheduler->running_thread = thread;
    }
    
    pthread_mutex_unlock(&scheduler->scheduler_mutex);
    return thread;
}

void block_thread(struct thread_scheduler *scheduler, struct TCB *thread) {
    pthread_mutex_lock(&scheduler->scheduler_mutex);
    
    if (scheduler->running_thread == thread) {
        scheduler->running_thread = NULL;
    }
    thread->state = 2; // Blocked state
    enqueue_thread(scheduler->blocked_queue, thread);
    
    pthread_mutex_unlock(&scheduler->scheduler_mutex);
}

void unblock_thread(struct thread_scheduler *scheduler, struct TCB *thread) {
    pthread_mutex_lock(&scheduler->scheduler_mutex);
    
    // Remove from blocked queue and add to ready queue
    // I did simplified implementation - in a real system would need
    // to search and remove from the blocked queue
    thread->state = 0; // Ready state
    enqueue_thread(scheduler->ready_queue, thread);
    
    pthread_mutex_unlock(&scheduler->scheduler_mutex);
}

void terminate_thread(struct thread_scheduler *scheduler, struct TCB *thread) {
    pthread_mutex_lock(&scheduler->scheduler_mutex);
    
    if (scheduler->running_thread == thread) {
        scheduler->running_thread = NULL;
    }
    thread->state = 3; // Terminated state
    scheduler->current_threads--;
    
    pthread_mutex_unlock(&scheduler->scheduler_mutex);
}

int scheduler_has_work(struct thread_scheduler *scheduler) {
    pthread_mutex_lock(&scheduler->scheduler_mutex);
    int has_work = !is_thread_queue_empty(scheduler->ready_queue) || scheduler->running_thread != NULL;
    pthread_mutex_unlock(&scheduler->scheduler_mutex);
    return has_work;
}

struct TCB *run_thread_to_completion(struct TCB *thread) {
    while (tcb_has_next_instruction(thread)) {
        size_t instr = tcb_next_instruction(thread);
        // Execute the instruction - this would need to be thread-safe
        // simulate execution
        printf("Thread %zu executing instruction %zu\n", thread->tid, instr);
        usleep(1000); // Small delay to simulate execution
    }
    return NULL; // Thread completed
}

struct TCB *run_thread_for_n_steps(struct TCB *thread, size_t n) {
    for (; n && tcb_has_next_instruction(thread); --n) {
        size_t instr = tcb_next_instruction(thread);
        // Execute the instruction
        printf("Thread %zu executing instruction %zu\n", thread->tid, instr);
        usleep(1000); // Small delay to simulate execution
    }
    
    if (tcb_has_next_instruction(thread)) {
        return thread;
    } else {
        return NULL; // Thread completed
    }
}

void *thread_execution_function(void *arg) {
    struct TCB *thread = (struct TCB *)arg;
    
    // Execute the thread
    while (tcb_has_next_instruction(thread)) {
        size_t instr = tcb_next_instruction(thread);
        // Execute the instruction - this need to be thread-safe
        printf("Thread %zu executing instruction %zu\n", thread->tid, instr);
        usleep(1000); // Small delay to simulate execution
    }
    
    return NULL;
}

struct PCB *run_process_multithreaded(struct PCB *pcb, int num_threads) {
    if (num_threads <= 0) num_threads = 1;
    
    // thread scheduler
    struct thread_scheduler *scheduler = create_thread_scheduler(num_threads);
    if (!scheduler) return NULL;
    
    // threads for this process
    for (int i = 0; i < num_threads; i++) {
        struct TCB *thread = create_thread(pcb);
        if (thread) {
            add_thread_to_process(pcb, thread);
            add_thread_to_scheduler(scheduler, thread);
        }
    }
    
    // Run the threads
    while (scheduler_has_work(scheduler)) {
        struct TCB *thread = get_next_thread(scheduler);
        if (thread) {
            struct TCB *result = run_thread_for_n_steps(thread, 1);
            if (result) {
                // Thread still has work, put it back in ready queue
                add_thread_to_scheduler(scheduler, result);
            } else {
                // Thread completed
                terminate_thread(scheduler, thread);
                remove_thread_from_process(pcb, thread);
                free_thread(thread);
            }
        }
        usleep(1000); // Small delay between thread switches
    }
    
    // Clean up scheduler
    free_thread_scheduler(scheduler);
    
    return pcb;
} 