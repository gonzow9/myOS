# myOS - Operating Systems Project

A comprehensive Operating Systems implementation project consisting of three progressive stages that build upon each other to create a functional operating system simulator.

## Table of Contents

- [Overview](#overview)
- [Project Structure](#project-structure)
- [Part 1: Shell Implementation](#part-1-shell-implementation)
- [Part 2: Process Scheduling](#part-2-process-scheduling)
- [Part 3: Memory Management](#part-3-memory-management)
- [Building and Running](#building-and-running)
- [Testing](#testing)
- [Features](#features)

## Overview

This project implements core operating system concepts through three progressive stages:

1. **Shell Implementation** - A command-line interpreter with basic shell functionality
2. **Process Scheduling** - Multi-process scheduling with various scheduling algorithms
3. **Memory Management** - Virtual memory management with page faults and memory allocation
4. **Multiprocessing** - multithreading

Each stages builds upon the previous one, creating a comprehensive operating system simulator written in C.

## Project Structure

```
myOS/
├── A1-2024/                    # Stage 1: Shell Implementation
│   ├── solution/               # Complete implementation
│   ├── starter-code/           # Base code for development
│   ├── test-cases/            # Basic shell command tests
│   └── Assignment_1_Fall2024.pdf
├── A2-2024/                   # Stage 2: Process Scheduling
│   ├── solution/              # Complete implementation
│   ├── test-cases/           # Scheduling algorithm tests
│   └── Assignment_2_Fall2024.pdf
└── A3-2024/                   # Stage 3: Memory Management
    ├── test-cases/            # Memory management tests
    └── Assignment_3_Fall2024.pdf
```

## Stage 1: Shell Implementation

### Features
- **Interactive Shell**: Command-line interface with prompt (`$`)
- **Batch Mode**: Execute commands from file input
- **Built-in Commands**:
  - `help` - Display available commands
  - `quit` - Exit the shell
  - `set VAR VALUE` - Set shell variables
  - `print VAR` - Print variable value
  - `run SCRIPT` - Execute script files
  - `echo TEXT` - Display text
  - `ls` - List directory contents
  - `mkdir DIR` - Create directories

### Core Components
- **`shell.c/h`**: Main shell interface and input parsing
- **`interpreter.c/h`**: Command interpretation and execution
- **`shellmemory.c/h`**: Variable storage and memory management

### Key Features
- Variable storage and retrieval
- Script execution capability
- Error handling for invalid commands
- Support for both interactive and batch modes

## Stage 2: Process Scheduling

### Scheduling Algorithms
- **FCFS (First-Come-First-Served)**: Basic queue-based scheduling
- **SJF (Shortest Job First)**: Priority scheduling by job duration
- **RR (Round Robin)**: Time-sliced scheduling with configurable quantum
- **Aging**: Priority-based scheduling with aging to prevent starvation

### Core Components
- **`pcb.c/h`**: Process Control Block implementation
- **`queue.c/h`**: Queue data structure for process management
- **`schedule_policy.c/h`**: Scheduling algorithm implementations

### Features
- Multi-process execution
- Process state management (ready, running, terminated)
- Configurable scheduling policies
- Process creation from script files
- Background process execution

### Process Management
- Each process has a unique PID
- Memory allocation tracking (base + bounds)
- Program counter management
- Duration estimation for SJF and aging algorithms

## Stage 3: Memory Management

### Features
- **Virtual Memory**: Page-based memory management
- **Page Faults**: Automatic page loading on demand
- **Memory Allocation**: Dynamic memory allocation for processes
- **Exec Command**: Execute programs with memory management
- **Run Command**: Alternative execution method

### Key Concepts
- Page-based memory organization
- On-demand loading of program pages
- Memory size configuration
- Page fault handling and resolution

### Test Cases
- **tc1, tc2**: Basic exec command testing (no page faults)
- **tc3**: Page fault handling
- **tc4**: Complex execution scenarios
- **tc5**: Run command testing

## Stage 4: Multiprocessing

## Building and Running

### Prerequisites
- GCC compiler
- Make build system
- Unix-like environment (Linux, macOS)

### Building
```bash
cd A1-2024
make
```

### Running
```bash
# Interactive mode
./mysh

# Batch mode
./mysh < input_file.txt

# With scheduling
./mysh
> exec program1 program2 program3
> POLICY_NAME  # FCFS, SJF, RR, AGING
```

## Testing

### Stage 1 Tests
Located in `A1-2024/test-cases/`:
- Basic command functionality
- Variable operations
- Script execution
- Error handling

### Stage 2 Tests
Located in `A2-2024/test-cases/`:
- Scheduling algorithm verification
- Multi-process scenarios
- Performance comparisons
- Background execution

### Stage 3 Tests
Located in `A3-2024/test-cases/`:
- Memory management scenarios
- Page fault handling
- Exec/run command testing

### Running Tests
```bash
# Example test execution
./mysh < test-cases/tc1.txt

# Compare with expected output
diff output.txt test-cases/tc1_result.txt
```

## Features

### Shell Capabilities
- Interactive command-line interface
- Batch processing
- Variable management
- Script execution
- Built-in commands
- Error handling

### Process Management
- Multiple scheduling algorithms
- Process creation and termination
- Priority-based scheduling
- Round-robin time slicing
- Aging prevention of starvation

### Memory Management
- Virtual memory simulation
- Page fault handling
- Dynamic memory allocation
- On-demand loading
- Memory size configuration

## Usage Examples

### Basic Shell Commands
```bash
$ help
$ set x 10
$ print x
$ echo Hello World
$ run script.txt
$ quit
```

### Process Scheduling
```bash
$ exec prog1 prog2 prog3
$ FCFS
# or
$ RR 5  # Round Robin with quantum 5
```

### Memory Management
```bash
$ exec program1 program2  # Programs loaded with page management
$ run program3           # Alternative execution method
```

## What I learnt

This project taught me fundamental OS concepts:
- **Process Management**: Creation, scheduling, and termination
- **Memory Management**: Virtual memory, paging, and allocation
- **System Calls**: Command interpretation and execution
- **Data Structures**: Queues, process control blocks, and memory tables
- **Algorithms**: Various scheduling policies and their trade-offs

