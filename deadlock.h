#ifndef DEADLOCK_H
#define DEADLOCK_H

#include "PCB.h"
#include "List.h"
#include <stdbool.h>

// Global resource arrays
extern int total_resources[MAX_RESOURCES];     // Total system resources
extern int available_resources[MAX_RESOURCES]; // Currently available resources

// Deadlock avoidance using Banker's Algorithm
bool bankers_algorithm(PCB **processes, int num_processes);

// Deadlock detection
bool detect_deadlock(PCB **processes, int num_processes);

// Deadlock prevention (e.g., resource ordering)
bool prevent_deadlock(PCB **processes, int num_processes);

int request_resources(PCB *proc, int request[MAX_RESOURCES], PCB **all_procs, int num_procs);
void release_resources(PCB *proc, int release[MAX_RESOURCES]);
void print_resource_state(PCB **processes, int num_processes);

#endif // DEADLOCK_H 