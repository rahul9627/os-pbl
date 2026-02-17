#ifndef _PCB_H_
#define _PCB_H_
#include <stdbool.h>
#include "List.h"


#define NUM_SEMAPHORE 5
#define NUM_READY_LIST 3
#define NUM_WAITING_LIST 2
#define MAX_RESOURCES 5 // Number of resource types

enum ProcState {
    RUNNING,
    READY,
    BLOCKED
};

enum WaitState {
    WAITING_SEND,
    WAITING_REPLY,
    WAITING_SEM
};

typedef struct PCB_s PCB;
struct PCB_s {
    int pid;
    int priority;
    enum ProcState state;
    enum WaitState waitState;
    char *proc_message;
    int msg_src;

    // Since a reply is handled differently than the a send, we must store it elsewhere
    char *reply_msg;
    int reply_src;
    // Resource allocation and request tracking for deadlock handling
    int allocated[MAX_RESOURCES]; // Resources currently allocated to this process
    int requested[MAX_RESOURCES]; // Resources currently requested by this process
};

typedef struct semaphore_t sem_t;
struct semaphore_t {
    bool sem_init;
    int sem_value;
    List *pList;        // Processes blocked on this semaphore
};

// Create a process and put it on the appropriate ready queue.
// Reports: success or failure, the pid of created process on success.
int create(int priority);

// Copy the currently running process and put it on the ready Q corresponding to the
//  original process' priority. Attempting to Fork the "init" process (see below) should fail.
// Reports: Success or failure, the pid of the resulting process on success.
int fork();

// Kill the named process and remove it from the system.
// Reports: Action taken as well as success or failure.
int kill(int pid);

// Kill the currently running process.
// Reports: Process scheduling information (which process now gets control of the cpu).
void exit_proc();

// Time quantum of the running process expires.
// Reports: Action taken (process scheduling information).
void quantum();

// Send a message to another process, block until reply.
// Reports: success or failure, scheduling information, and reply source and text (once
//  reply arrives).
int send(int pid, char *msg);

// Receive a message, block until one arrives
// Reports: Scheduling information, message text, source of message.
void receive();

// Unblocks sender and delivers reply.
// Reports: Success or failure.
int reply(int pid, char *msg);

// Initialize the named semaphore with the value given. IDs can take a value from 0 to 4. 
//  This can only be done once for a semaphore - subsequent attempts result in error.
// Reports: Action taken as well as success or failure.
int new_Sem(int semaphore, unsigned int init);

// Execute the semaphore P operation on behalf of the running process. Assume semaphore 
//  IDs to be numbered 0 through 4.
// Reports: Action taken (blocked or not) as well as success or failure.
int sem_P(int sem_id);

// Execute the semaphore V operation on behalf of the running process. Assume semaphore 
//  IDs to be numbered 0 through 4.
int sem_V(int sem_id);

// Dump complete state information of process to screen.
void procinfo(int pid);

// Display all process queues and their contents
void totalinfo();


// --------- -UTILITY FUNCTIONS----------

// Dequeue from list
static void* dequeue(List * list);

// Initialize all lists
void initProgram(List * readyTop, List * readyNorm, List * readyLow, List * readyInit, List * waitingSend, List * waitingReceive);

// Take input from the keyboard
static void checkInput();

// Free a process control block
static void freeProcess(PCB *pList);

static PCB* nextProcess();

// Search the relevant queues for the given pid
static PCB *findProcess(int pid);

// Helper function to print process information to the screen
static void procinfo_helper(PCB *process);

static bool readyListEmpty();



#endif