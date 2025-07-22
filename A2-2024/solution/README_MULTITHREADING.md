# Multi-Threading Support for myOS

This describes the multi-threading capabilities that have been added to the OS.

## Overview

The multi-threading system allows processes to be executed using multiple threads, providing concurrent execution within a single process. This is implemented through:

1. **Thread Control Blocks (TCB)** - Each thread has its own control block
2. **Thread Scheduler** - Manages thread execution and scheduling
3. **Process-Level Thread Management** - Each process can contain multiple threads
4. **Thread-Safe Operations** - Mutex-based synchronization

## Key Components

### Thread Control Block (TCB)
- `tid`: Unique thread identifier
- `parent_pcb`: Reference to the parent process
- `pc`: Program counter for this thread
- `stack_base` and `stack_size`: Thread stack information
- `state`: Thread state (ready, running, blocked, terminated)
- `next`: For queue management

### Process Control Block (PCB) Updates
- `thread_count`: Number of threads in the process
- `threads`: Linked list of threads
- `process_mutex`: Mutex for process-level synchronization

### Thread Scheduler
- Manages ready and blocked thread queues
- Handles thread state transitions
- Provides thread scheduling algorithms
- Supports configurable maximum thread count

## Usage

### Enabling Multi-Threading
To enable multi-threading, add "MT" as the last argument to the exec command:

```bash
exec program1 FCFS MT
```

This will run `program1` using the FCFS scheduling policy with multi-threading enabled.

### Thread Configuration
By default, each process runs with 4 threads. This can be modified in the `runSchedule` function in `interpreter.c`.

## Building

### Main Shell
```bash
make
```

### Test Program
```bash
make test_thread
./test_thread
```

## Features

### 1. Thread Creation and Management
- Automatic thread creation for processes
- Thread lifecycle management (create, run, block, terminate)
- Thread cleanup and resource management

### 2. Thread Scheduling
- Round-robin thread scheduling
- Thread state management
- Support for thread blocking and unblocking

### 3. Synchronisation
- Process-level mutex for thread safety
- Scheduler-level mutex for concurrent access
- Thread-safe queue operations

### 4. Resource Management
- Automatic thread cleanup
- Memory management for thread stacks
- Process cleanup includes thread cleanup

## Implementation Details

### Thread Execution Model
Each thread executes instructions from the same process code but maintains its own program counter. This allows for:

- Concurrent execution of different parts of the same program
- Shared access to process memory
- Independent thread state management

### Scheduling Algorithm
The thread scheduler uses a simple round-robin approach:

1. Threads are added to the ready queue
2. Scheduler selects the next thread from the ready queue
3. Thread executes for a time quantum
4. If thread has more work, it's added back to the ready queue
5. If thread completes, it's terminated and cleaned up

### Thread States
- **Ready (0)**: Thread is ready to execute
- **Running (1)**: Thread is currently executing
- **Blocked (2)**: Thread is waiting for a resource
- **Terminated (3)**: Thread has completed execution

## Limitations and Future Improvements

### Current Limitations
1. Limited to 4 threads per process (configurable)
2. Simple round-robin scheduling
3. No thread priorities
4. No inter-thread communication mechanisms

### Potential Improvements
1. **Thread Priorities**: Implement priority-based scheduling
2. **Thread Communication**: Add mutex, semaphore, and condition variable support
3. **Dynamic Thread Creation**: Allow runtime thread creation
4. **Thread Pools**: Implement thread pooling for better resource management
5. **Load Balancing**: Add load balancing across multiple processes

## Testing

The system includes a test program (`test_thread.c`) that demonstrates:

- Process creation with multiple threads
- Thread execution and scheduling
- Resource cleanup

To run the test:
```bash
make test_thread
./test_thread
```

## Integration with Existing System

The multi-threading system is backward compatible:

- Processes without "MT" flag run in single-threaded mode
- Existing scheduling policies work with both single and multi-threaded processes
- All existing shell commands continue to work as before

## Thread Safety Considerations

The implementation includes several thread safety measures:

1. **Process Mutex**: Protects process-level data structures
2. **Scheduler Mutex**: Protects scheduler operations
3. **Queue Mutex**: Protects queue operations (inherited from existing queue implementation)
4. **Memory Allocation**: Thread-safe memory allocation for thread structures

## Performance Considerations

- Thread creation overhead is minimal
- Context switching between threads is fast
- Memory usage scales with the number of threads
- CPU utilisation improves with multi-threading for I/O-bound tasks
