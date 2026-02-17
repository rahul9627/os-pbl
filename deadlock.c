#include "deadlock.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Define and initialize global resource arrays
int total_resources[MAX_RESOURCES] = {10, 10, 10, 10, 10};     // Example: 10 of each resource
int available_resources[MAX_RESOURCES] = {10, 10, 10, 10, 10}; // Initially all available

// Helper: Check if all needs of a process can be satisfied by work[]
static bool can_satisfy(PCB *proc, int work[MAX_RESOURCES]) {
    for (int i = 0; i < MAX_RESOURCES; ++i) {
        if (proc->requested[i] > work[i])
            return false;
    }
    return true;
}

// Banker's Algorithm: returns true if system is in a safe state
bool bankers_algorithm(PCB **processes, int num_processes) {
    int work[MAX_RESOURCES];
    bool finish[num_processes];
    memcpy(work, available_resources, sizeof(work));
    for (int i = 0; i < num_processes; ++i) finish[i] = false;

    int count = 0;
    while (count < num_processes) {
        bool found = false;
        for (int i = 0; i < num_processes; ++i) {
            if (!finish[i] && can_satisfy(processes[i], work)) {
                // Pretend to allocate all needed resources
                for (int j = 0; j < MAX_RESOURCES; ++j)
                    work[j] += processes[i]->allocated[j];
                finish[i] = true;
                found = true;
                ++count;
            }
        }
        if (!found) break;
    }
    // If all processes can finish, system is safe
    for (int i = 0; i < num_processes; ++i)
        if (!finish[i]) return false;
    return true;
}

// Deadlock Detection: returns true if deadlock exists
bool detect_deadlock(PCB **processes, int num_processes) {
    int work[MAX_RESOURCES];
    bool finish[num_processes];
    memcpy(work, available_resources, sizeof(work));
    for (int i = 0; i < num_processes; ++i) {
        // If process has no allocated resources, it's not deadlocked
        bool has_alloc = false;
        for (int j = 0; j < MAX_RESOURCES; ++j)
            if (processes[i]->allocated[j] > 0) has_alloc = true;
        finish[i] = !has_alloc;
    }
    int count = 0;
    while (count < num_processes) {
        bool found = false;
        for (int i = 0; i < num_processes; ++i) {
            if (!finish[i] && can_satisfy(processes[i], work)) {
                for (int j = 0; j < MAX_RESOURCES; ++j)
                    work[j] += processes[i]->allocated[j];
                finish[i] = true;
                found = true;
                ++count;
            }
        }
        if (!found) break;
    }
    // If any process is unfinished, deadlock exists
    for (int i = 0; i < num_processes; ++i)
        if (!finish[i]) return true;
    return false;
}

// Deadlock Prevention: enforce resource ordering
// Returns true if request is allowed, false if it violates ordering
bool prevent_deadlock(PCB **processes, int num_processes) {
    // Example: enforce that resources must be requested in increasing order
    for (int i = 0; i < num_processes; ++i) {
        int last = -1;
        for (int j = 0; j < MAX_RESOURCES; ++j) {
            if (processes[i]->requested[j] > 0) {
                if (j < last) return false; // Out of order
                last = j;
            }
        }
    }
    return true;
}

// Request resources for a process
int request_resources(PCB *proc, int request[MAX_RESOURCES], PCB **all_procs, int num_procs) {
    // Update requested array
    for (int i = 0; i < MAX_RESOURCES; ++i)
        proc->requested[i] = request[i];

    // Check prevention and avoidance
    if (!prevent_deadlock(all_procs, num_procs) || !bankers_algorithm(all_procs, num_procs)) {
        // Deny request
        return -1;
    }

    // Grant request
    for (int i = 0; i < MAX_RESOURCES; ++i) {
        available_resources[i] -= request[i];
        proc->allocated[i] += request[i];
        proc->requested[i] = 0;
    }
    return 0;
}

// Release resources for a process
void release_resources(PCB *proc, int release[MAX_RESOURCES]) {
    for (int i = 0; i < MAX_RESOURCES; ++i) {
        proc->allocated[i] -= release[i];
        available_resources[i] += release[i];
        if (proc->allocated[i] < 0) proc->allocated[i] = 0;
    }
}

// Print current resource allocation state (for debugging/frontend)
void print_resource_state(PCB **processes, int num_processes) {
    printf("Available resources: ");
    for (int i = 0; i < MAX_RESOURCES; ++i) printf("%d ", available_resources[i]);
    printf("\n");
    for (int p = 0; p < num_processes; ++p) {
        printf("Process %d: Allocated:", processes[p]->pid);
        for (int i = 0; i < MAX_RESOURCES; ++i) printf(" %d", processes[p]->allocated[i]);
        printf(" | Requested:");
        for (int i = 0; i < MAX_RESOURCES; ++i) printf(" %d", processes[p]->requested[i]);
        printf("\n");
    }
} 