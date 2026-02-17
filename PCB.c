#include "PCB.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "deadlock.h"

static PCB *CURRENT = NULL;
static PCB *INIT = NULL;
static unsigned int PID_CURR = 0;
static bool initMade = false;
static unsigned int SEM_NUM = 0;
static int proc_count = 0;
static bool exit_loop = false;

static sem_t sem_array[NUM_SEMAPHORE]; 
static List * ready_lists[NUM_READY_LIST];      // 0 - high priority, 1 - normal priority, 2 - low priority
static List * waiting_lists[NUM_WAITING_LIST];  // 0 - waiting for send, 1 - waiting for reply

static PCB* pcb_array[100]; // For up to 100 processes
static int pcb_array_count = 0;

static void exit_sim();

// Create a process and put it on the appropriate ready queue.
// Reports: success or failure, the pid of created process on success.
int create(int priority) {

    // Check that the given priority is valid
    if (priority < 0 || priority > 2 && initMade == true) {
        return -1;
    }

    PCB *newPCB = malloc(sizeof(PCB));
    // If allocation fails
    if (newPCB == NULL) {
        printf("Error: Memory allocation failed\n");
        return -1;
    }

    // Set member variables
    newPCB->pid = PID_CURR;
    PID_CURR++;
    newPCB->priority = priority;
    newPCB->waitState = 2;
    newPCB->msg_src = -1;

    // If there are no processes currently running
    if (CURRENT == NULL) {
        newPCB->state = RUNNING;
        CURRENT = newPCB;
    }
    // If the currently running process is the init process
    else if (CURRENT->priority == 3) {
        INIT->state = READY;
        newPCB->state = RUNNING;
        if(List_append(ready_lists[CURRENT->priority], CURRENT) != -1) {
            CURRENT = newPCB;
        }
        else {
            return -1;
        }
    }
    else {
        newPCB->state = READY;
        if(List_append(ready_lists[newPCB->priority], newPCB) == -1) {
            return -1;
        }
    }

    // Return pid of the created process
    proc_count++;
    pcb_array[pcb_array_count++] = newPCB;
    return newPCB->pid;
}

// Copy the currently running process and put it on the ready Q corresponding to the
// original process' priority. Attempting to Fork the "init" process (see below) should fail.
// Reports: Success or failure, the pid of the resulting process on success.
// Returns -1 on failure, the pid on success
int fork() {
    
    if (CURRENT == NULL || CURRENT->pid == 0) {
        printf("Error: Cannot fork the init process\n");
        return -1;
    }

    // Create the new process
    PCB *newPCB = malloc(sizeof(PCB));
    newPCB->pid = PID_CURR;
    PID_CURR++;
    newPCB->priority = CURRENT->priority;
    newPCB->state = READY;
    newPCB->waitState = CURRENT->waitState;

    // Enqueue the new process
    if(List_append(ready_lists[newPCB->priority], newPCB) == -1) {
        return -1;
    }
    else {
        proc_count++;
        pcb_array[pcb_array_count++] = newPCB;
        return newPCB->pid;  
    }
}

// Kill the named process and remove it from the system.
// Reports: Action taken as well as success or failure.
int kill(int pid) {

    PCB *toKill = NULL;

    // If user is requesting to kill the init process
    if (pid == INIT->pid) {
        if(proc_count == 1) {
            // call exit
            printf("Init process killed \n");
            exit_sim();
            return 1;
        }
        printf("Error: Cannot kill init process\n");
        return -1;
    }

    if (CURRENT != NULL) {
        // If we are requesting to kill the current process
        if (CURRENT->pid == pid) {
            toKill = CURRENT;
            CURRENT = nextProcess();
            freeProcess(toKill);
            printf("Process %i killed\n", pid);
            proc_count--;
            return 1;
        }
    }

    toKill = findProcess(pid); // Find the desired process. The list's "current" node is to be removed now

    if (toKill != NULL) {
        // If the process is on a ready queue 
        if (toKill->state == READY) {
            List_remove(ready_lists[toKill->priority]);
            freeProcess(toKill);
        // If the process is on a waiting queue
        } else {
            // If the process is waiting on a send
            if (toKill->waitState == WAITING_SEND) {
                List_remove(waiting_lists[0]);
                freeProcess(toKill);
            }
            else if (toKill->waitState == WAITING_REPLY) {
                List_remove(waiting_lists[1]);
                freeProcess(toKill);
            }
            // If the process is on a semaphore list
            else {
                for (int i = 0; i < 5; i++) {
                    if(sem_array[i].sem_init == true) {
                        if(List_curr(sem_array[i].pList) == toKill) {
                            List_remove(sem_array[i].pList);
                            freeProcess(toKill);
                            sem_array[i].sem_value++;
                            break;
                        }
                    }
                }
            }
        }
        printf("Process %i killed\n", pid);
        proc_count--;
        return 1;
    }

    // If we reach this line, process removal has failed
    printf("Error: PCB not found\n");
    return -1;
}

// Kill the currently running process.
// Reports: Process scheduling information (which process now gets control of the cpu).
void exit_proc() {

    if (CURRENT != NULL) {
        kill(CURRENT->pid);
    }
}

// Time quantum of the running process expires.
// Reports: Action taken (process scheduling information).
void quantum() {
    
    if(CURRENT == INIT) {
        printf("--New Current Process: \n");
        procinfo_helper(CURRENT);
        return;
    }

    CURRENT->state = READY;
    printf("--Expired process: \n");
    procinfo_helper(CURRENT);

    if (proc_count == 2 || readyListEmpty()) {
        CURRENT->state = RUNNING;
        printf("--New Current Process: \n");
        procinfo_helper(CURRENT);
        return;
    }

    // Find the next process to run, remove it from the appropriate queue
    PCB *temp = nextProcess();

    // Remove the new process from the appropriate queue
    findProcess(temp->pid);
    List_remove(ready_lists[temp->priority]);

    // Enqueue the current process to the appropriate queue, change to the new process
    List_append(ready_lists[CURRENT->priority], CURRENT);
    CURRENT = temp;

}

// Send a message to another process, block until reply.
// Reports: success or failure, scheduling information, and reply source and text (once
//  reply arrives).
int send(int pid, char *msg) {
    
    // If we try to send to the currently running process, operation fails
    if (CURRENT->pid == pid) {
        printf("Error: Cannot send to currently running process\n");
        return -1;
    }

    // Look for the target
    PCB* target = findProcess(pid);
    if (target == NULL) {
        printf("Error: PCB not found\n");
        return -1;
    }

    // If the target already has a message queued
    if (target->proc_message != NULL) {
        printf("Error: Target already has a message queued\n");
        return -1;
    }

    // We must not block the init process
    if (target->state != BLOCKED && CURRENT == INIT) {
        printf("Error: Cannot block the init process\n");
        return -1;
    }
    else if (target->state == BLOCKED && target->waitState != WAITING_SEND && CURRENT == INIT) {
        printf("Error: Cannot block the init process\n");
        return -1;
    }

    // Check if the receiving process is blocked
    if (target->state == BLOCKED) {
        // Check if the receiving process is waiting for a send
        if (target->waitState == WAITING_SEND) {
            
            // Give the target process the message
            target->proc_message = strdup(msg);
            target->msg_src = CURRENT->pid;
            target->state = READY;

            List_remove(waiting_lists[0]);  // Remove target process from the waiting queue (it is already waiting_list[1]'s current process)
            if (List_append(ready_lists[target->priority], target) == -1) {
                return -1;
            } 

            // If the currently running process is the init process, make the target the newly running process
            if (CURRENT == INIT) {
                dequeue(ready_lists[target->priority]);
                target->state = RUNNING;
                CURRENT = target;
            }

            // Move the current process to waiting list
            CURRENT->state = BLOCKED;
            CURRENT->waitState = WAITING_REPLY;
            if(List_append(waiting_lists[1], CURRENT) == -1) {
                return -1;
            }

            printf("--Blocking process: \n");
            procinfo_helper(CURRENT);
                    
            // Run the next process in the queue
            CURRENT = nextProcess();



            // Return success
            return 1;
        }

    }
    // If the target process is not blocked, or is waiting for a receive:

    if (target->pid == CURRENT->msg_src) {
        printf("Error: Target process is waiting for a receive from current process\n");
        return -1;
    }

    // Move the current process to waiting list
    CURRENT->state = BLOCKED;
    CURRENT->waitState = WAITING_REPLY;
    if(List_append(waiting_lists[1], CURRENT) == -1) {
        return -1;
    }

    // Give the target process the message
    target->proc_message = strdup(msg);
    target->msg_src = CURRENT->pid;

    printf("--Blocking process: \n");
    procinfo_helper(CURRENT);
            
    // Run the next process in the queue
    CURRENT = nextProcess();
    
    return 1;
}

// Receive a message, block until one arrives
// Reports: Scheduling information, message text, source of message.
void receive() {

    if (CURRENT == INIT) {
        // We should never block the init process
        if (CURRENT->proc_message == NULL) {
            printf("Error: Cannot block the init process\n");
            return;
        }
    }
    
    // If the new process has a message, print it 
    if (CURRENT->proc_message != NULL) {  
        
        printf("Message received from process %i\n", CURRENT->msg_src);
        printf("Received Message: %s\n", CURRENT->proc_message);

        // Clear the message information from the current process
        free(CURRENT->proc_message);
        CURRENT->proc_message = NULL;
        CURRENT->msg_src = -1;

        return;
    }
    // If there's no messages to receive, move the process to the waiting list
    else {
        
        // Move current process to the waiting list
        CURRENT->state = BLOCKED;
        CURRENT->waitState = WAITING_SEND;
        List_append(waiting_lists[0], CURRENT);
        printf("--Blocking process: \n");
        procinfo_helper(CURRENT);


        // Run the next process in the queue
        CURRENT = nextProcess();
        
        return;
    }
}

// Unblocks sender and delivers reply.
// Reports: Success or failure.
int reply(int pid, char *msg) {
    
    // If we try to send to the currently running process, operation fails
    if (CURRENT->pid == pid) {
        printf("Error: Cannot reply to currently running process\n");
        return -1;
    }

    // Find the target in a list
    PCB* target = findProcess(pid);
    if(target == NULL) {
        printf("Error: PCB not found\n");
        return -1;
    }

    // If the target already has a message queued
    if (target->reply_msg != NULL) {
        printf("Error: Target already has a message queued\n");
        return -1;
    }

    // If the target has not been blocked by a send, we cannot reply to it
    if (target->state != BLOCKED || (target->state == BLOCKED && target->waitState != WAITING_REPLY)) {
        printf("Error: Target is not waiting for a reply\n");
        return -1;
    }

    // If the target doesn't currently hold a message, reply with a message:
    target->reply_msg = strdup(msg);
    target->reply_src = CURRENT->pid;
        
    // Remove the target from the waiting list
    List_remove(waiting_lists[1]);    
    target->state = READY;
    if (List_append(ready_lists[target->priority], target) == -1) {
        return -1;
    }

    // If the current process is the INIT process
    if (CURRENT == INIT) {
        CURRENT = nextProcess();
    }
    
    // Return success
    return 1;
}

// Initialize the named semaphore with the value given. IDs can take a value from 0 to 4. 
//  This can only be done once for a semaphore - subsequent attempts result in error.
// Reports: Action taken as well as success or failure.
int new_Sem(int sem_id, unsigned int init) {

    // Check for valid semaphore ID
    if (sem_id > 4 || sem_id < 0) {
        printf("Error: Not a valid semaphore ID\n");
        return -1;
    }
    // Check that we have not created too many semaphores
    if (SEM_NUM >= NUM_SEMAPHORE) {
        printf("Error: Created too many semaphores\n");
        return -1;
    }
    // Check that we have not already created a semaphore with the given ID
    if (sem_array[sem_id].pList != NULL) {
        printf("Error: This semaphore has already been created!\n");
        return -1;
    }
    if (init < 0) {
        printf("Error: Invalid initialization value\n");
        return -1;
    }

    sem_array[sem_id].sem_value = init;
    sem_array[sem_id].sem_init = true;
    sem_array[sem_id].pList = List_create();
    SEM_NUM++;

}

// Execute the semaphore P operation on behalf of the running process. Assume semaphore 
//  IDs to be numbered 0 through 4.
// Reports: Action taken (blocked or not) as well as success or failure.
int sem_P(int sem_id) {
    
    // Check for valid semaphore ID
    if (sem_id > 4 || sem_id < 0) {
        printf("Error: Invalid semaphore ID\n");
        return -1;
    }

    // Check if semaphore has been created yet
    if (sem_array[sem_id].pList == NULL) {
        printf("Error: Semaphore has not been created yet\n");
        return -1;
    }

    // Cannot block the init process
    if (CURRENT == INIT) {
        printf("Error: Cannot block the init process\n");
        return -1;
    }

    // If sem value is greater than 0, decrement semaphore and return
    if (sem_array[sem_id].sem_value > 0) {
        sem_array[sem_id].sem_value--;
        return 1;
    }
    else {

        sem_array[sem_id].sem_value--;  // Decrement semaphore value
        
        // Update process information
        CURRENT->waitState = WAITING_SEM;
        CURRENT->state = BLOCKED;

        // Add process to the waiting list of the semaphore
        List_append(sem_array[sem_id].pList, CURRENT);

        // Output action taken
        printf("Blocking process: \n");
        procinfo_helper(CURRENT);

        // Run next process
        CURRENT = nextProcess();

        return 1;
    }

}

// Execute the semaphore V operation on behalf of the running process. Assume semaphore 
//  IDs to be numbered 0 through 4.
// Reports: Action taken, as well as success or failure
int sem_V(int sem_id) {
    
    // Check for valid semaphore ID
    if (sem_id > 4 || sem_id < 0) {
        printf("Error: Invalid semaphore ID\n");
        return -1;
    }

    // Check if semaphore has been created yet
    if (sem_array[sem_id].pList == NULL) {
        printf("Error: Semaphore has not been created yet\n");
        return -1;
    }

    // Increment semaphore value
    sem_array[sem_id].sem_value++;

    // If there are processes waiting on this semaphore, unblock one process
    if (sem_array[sem_id].sem_value <= 0) {
        
        PCB *temp = (PCB *)dequeue(sem_array[sem_id].pList);
        temp->state = READY;

        
        // If the init process is currently running, set temp to running
        if(CURRENT == INIT) {
            INIT->state = READY;
            temp->state = RUNNING;
            CURRENT = temp;
        }
        // Else, send to appropriate queue
        else {
            List_append(ready_lists[temp->priority], temp); 
        }

        // Output action taken
        printf("Process unblocked: \n");
        procinfo_helper(temp);

        return 1;
    }
    // Otherwise there were no processes waiting on this semaphore
    else {
        printf("No processes waiting on this semaphore\n");
        return 1;
    }
    
}

// Dump complete state information of process to screen.
void procinfo(int pid) {

    printf("---PROCESS INFO---\n");

    PCB *temp = NULL;

    // Either the process is the currently running process, or it is stored in a list
    if (CURRENT == NULL || CURRENT->pid != pid) {
        temp = findProcess(pid);
    } else if (CURRENT->pid == pid) {
        temp = CURRENT;
    }

    if (temp != NULL) {
        procinfo_helper(temp);
    }
    else {
        printf("Error: Process not found\n");
    }
}

// Display all process queues and their contents
void totalinfo() {
    
    printf("---TOTAL INFO---\n");

    // Display the currently running process
    if (CURRENT != NULL) {
        printf("--Current Process:\n");
        procinfo_helper(CURRENT);
    }

    // Display the ready lists
    for (int i = 0; i <= 2; i++) {
        List_first(ready_lists[i]);
        printf("--Ready List %i:\n", i);
        while (ready_lists[i]->current != NULL) {
            // Print process info
            PCB *processPointer = ready_lists[i]->current->item;
            procinfo_helper(processPointer);
            // Advance
            ready_lists[i]->current = ready_lists[i]->current->next;
        }
    }

    // Display the waiting lists
    for (int i = 0; i <= 1; i++) {
        List_first(waiting_lists[i]);

        // For readability:
        if (i == 0) {
            printf("--Waiting List for Send: \n");
        }
        else {
            printf("--Waiting List for Reply: \n");
        }

        while (waiting_lists[i]->current != NULL) {
            // Print process info
            PCB *processPointer = waiting_lists[i]->current->item;
            procinfo_helper(processPointer);
            // Advance
            waiting_lists[i]->current = waiting_lists[i]->current->next;
        }
    }

    // Display the semaphore lists
    for (int i = 0; i < 5; i++) {
        if (sem_array[i].pList != NULL) {
            
            List_first(sem_array[i].pList);
            printf("--Semaphore List %i:\n", i);
            while (sem_array[i].pList->current != NULL) {
                // Print process info
                PCB *processPointer = sem_array[i].pList->current->item;
                procinfo_helper(processPointer);
                // Advance
                sem_array[i].pList->current = sem_array[i].pList->current->next;
            }
        }
    }

}


// PRIVATE FUNCTIONS

// Dequeue from list
static void * dequeue(List * list) {
    List_first(list);
    void *ret = List_remove(list);
    return ret;
}

// Initialize all lists
void initProgram(List * readyTop, List * readyNorm, List * readyLow, List * readyInit, List * waitingSend, List * waitingReceive) {

    ready_lists[0] = readyTop;
    ready_lists[1] = readyNorm;
    ready_lists[2] = readyLow;
    ready_lists[3] = readyInit;

    waiting_lists[0] = waitingSend;
    waiting_lists[1] = waitingReceive;

    for (int i = 0; i < 5; i++) {
        sem_array[i].pList = NULL;    
        sem_array[i].sem_init = false;
    }

    // Initialize the special init process
    INIT = malloc(sizeof(PCB));
    INIT->pid = PID_CURR;
    PID_CURR++;
    INIT->priority = 3;
    INIT->state = RUNNING;
    CURRENT = INIT;
    initMade = true;
    proc_count++;
    pcb_array[pcb_array_count++] = INIT;
    exit_loop = false;

    printf("Simulation started. Waiting for input types...\n");
    // Start the input loop
    while(!exit_loop) {
        checkInput();
    }
    printf("Exiting Simulation!\n");
    printf("Simulation complete. Waiting for input types...\n");
}

static void checkInput() {
    char input[20];
    char msg[256];
    char int_in[256];
    int int_input;
    int int_input2;
    int rv;
    if (fgets(input, 20, stdin) == NULL) {
        exit_sim();
        return;
    }
    // fflush(stdin);
    char command = input[0];
    printf("---------------------------------------------------------------------------\n");
    switch (command) {
        case 'C':
            printf("Enter process priority (0 = high, 1 = norm, 2 = low): ");
            scanf("%d", &int_input);
            int new_pid = create(int_input);
            if (new_pid == -1) {
                printf("Failure: Could not create\n");
            }
            else {
                printf("New process ID: %i\n", PID_CURR-1);
                printf("Success: Create complete\n");
                PCB* new_pcb = findProcess(PID_CURR-1);
                if (new_pcb) pcb_array[pcb_array_count++] = new_pcb;
            }
            break;
        case 'F':
            rv = fork();
            if(rv == -1) {
                printf("Failure: Could not fork\n");
            }
            else {
                printf("New process ID: %i\n", rv);
                printf("Success: Fork complete\n");
            }
            break;
        case 'K':
            printf("Enter process ID: ");
            scanf("%d", &int_input);
            if (int_input > PID_CURR || int_input < 0) {
                printf("Failure: Invalid input\n");
            } 
            else if (kill(int_input) == -1) {
                printf("Failure: Could not kill\n");
            }
            else {
                printf("Success: Kill complete\n");
            }
            break;
        case 'E':
            exit_proc();
            break;
        case 'Q':
            quantum();
            break;
        case 'S':
            char command;
            printf("Enter process ID of receiver: ");
            fgets(int_in, 256, stdin);
            int_input = atoi(&int_in[0]);
            printf("Enter a message: ");
            // scanf("%s", msg);
            fflush(stdin);
            fgets(msg, 256, stdin);
            command = msg[0];
            if(send(int_input, msg) == -1) {
                printf("Failure: Could not send\n");
            }
            else {
                printf("Success: Send complete\n");
            }
            break;
        case 'R':
            receive();
            break;
        case 'Y':
            printf("Enter process ID to reply to: ");
            fgets(int_in, 256, stdin);
            int_input = atoi(&int_in[0]);
            printf("Enter a message: ");
            // scanf("%s", msg);
            fflush(stdin);
            fgets(msg, 256, stdin);
            command = msg[0];
            // unblock sender
            if (reply(int_input, msg) == -1) {
                printf("Failure: Could not reply\n");
            }
            else {
                printf("Success: Reply complete\n");
            }
            break;
        case 'N':
            printf("Enter the new semaphore ID: ");
            scanf("%d", &int_input);
            printf("Enter initial value of new semaphore: ");
            scanf("%d", &int_input2);
            // need to figure out number
            if(new_Sem(int_input, int_input2) == -1) {
                printf("Failure: Semaphore was not created\n");
            }
            else {
                printf("Success: Semaphore created\n");
            }
            break;
        case 'P':
            printf("Enter a semaphore ID: ");
            scanf("%d", &int_input);
            if(sem_P(int_input) == -1) {
                printf("Failure: Could not execute semaphore P\n");
            }
            else {
                printf("Success: Semaphore P executed\n");
            }
            break;
        case 'V':
            printf("Enter a semaphore ID: ");
            scanf("%d", &int_input);
            if(sem_V(int_input) == -1) {
                printf("Failure: Could not execute semaphore V\n");
            }
            else {
                printf("Success: Semaphore V executed\n");
            }
            break;
        case 'I':
            printf("Enter a process ID: ");
            scanf("%d", &int_input);
            procinfo(int_input);
            break;
        case 'T':
            totalinfo();
            print_resource_state(pcb_array, pcb_array_count);
            break;
        case 'D':
            if (detect_deadlock(pcb_array, pcb_array_count)) {
                printf("Deadlock detected!\n");
            } else {
                printf("No deadlock detected.\n");
            }
            break;
        case 'W':
            printf("Enter process ID: ");
            scanf("%d", &int_input);
            PCB* req_pcb = findProcess(int_input);
            if (!req_pcb) {
                printf("Process not found.\n");
                break;
            }
            printf("Enter 5 resource request values (comma separated, e.g. 1,0,0,0,0): ");
            int reqs[MAX_RESOURCES];
            for (int i = 0; i < MAX_RESOURCES; ++i) scanf("%d%*[, ]", &reqs[i]);
            int req_result = request_resources(req_pcb, reqs, pcb_array, pcb_array_count);
            if (req_result == 0) {
                printf("Resources granted.\n");
            } else {
                printf("Request denied (would cause deadlock or violate prevention rules).\n");
            }
            break;
        case 'w':
            printf("Enter process ID: ");
            scanf("%d", &int_input);
            PCB* rel_pcb = findProcess(int_input);
            if (!rel_pcb) {
                printf("Process not found.\n");
                break;
            }
            printf("Enter 5 resource release values (comma separated, e.g. 1,0,0,0,0): ");
            int rels[MAX_RESOURCES];
            for (int i = 0; i < MAX_RESOURCES; ++i) scanf("%d%*[, ]", &rels[i]);
            release_resources(rel_pcb, rels);
            printf("Resources released.\n");
            break;
    } 
    
    // To improve the readability of our outputs
    if (command == 'E' || command == 'F' || command == 'Q' || command == 'R' || command == 'T' || command == 'S' || command == 'Y') {
        printf("---------------------------------------------------------------------------\n");
    }
    
    printf("Waiting for input types...\n");
}

// Free a process control block
static void freeProcess(PCB *process) {
    
    if (process->proc_message != NULL) {
       free(process->proc_message);
       process->proc_message = NULL;
    }
    if (process->reply_msg != NULL) {
        free(process->reply_msg);
        process->reply_msg = NULL;
    }
    free(process);
    process = NULL;
}

// Called whenever we switch to a new process.
// Outputs process scheduling information.
static PCB* nextProcess() {
    
    if (List_count(ready_lists[0]) != 0) {
        PCB *ret = dequeue(ready_lists[0]);
        ret->state = RUNNING;
        printf("--New Current Process: \n");
        procinfo_helper(ret);

        // If ret holds a reply, print it to the screen immediately
        if (ret->reply_msg != NULL) {
            printf("Reply received from process %i\n", ret->reply_src);
            printf("Reply message: %s\n", ret->reply_msg);
            ret->reply_src = -1;
            free(ret->reply_msg);
            ret->reply_msg = NULL;
        }
        return ret;
    }
    else if(List_count(ready_lists[1]) != 0) {
        PCB *ret = dequeue(ready_lists[1]);
        ret->state = RUNNING;
        printf("--New Current Process: \n");
        procinfo_helper(ret);

        // If ret holds a reply, print it to the screen immediately
        if (ret->reply_msg != NULL) {
            printf("Reply received from process %i\n", ret->reply_src);
            printf("Reply message: %s\n", ret->reply_msg);
            ret->reply_src = -1;
            free(ret->reply_msg);
            ret->reply_msg = NULL;
        }
        return ret;
    }
    else if(List_count(ready_lists[2]) != 0) {
        PCB *ret = dequeue(ready_lists[2]);
        ret->state = RUNNING;
        printf("--New Current Process: \n");
        procinfo_helper(ret);

        // If ret holds a reply, print it to the screen immediately
        if (ret->reply_msg != NULL) {
            printf("Reply received from process %i\n", ret->reply_src);
            printf("Reply message: %s\n", ret->reply_msg);
            ret->reply_src = -1;
            free(ret->reply_msg);
            ret->reply_msg = NULL;
        }
        return ret;
    }
    else {
        INIT->state = RUNNING;
        printf("--New Current Process: \n");
        procinfo_helper(INIT);
        return INIT;
    }
}

// Search the relevant queues for the given pid.
// The queue's current node will now be the desired process.
static PCB* findProcess(int pid) {

    // If we search for the init process
    if (pid == 0) {
        return INIT;
    }

    // Search the ready lists
    for (int i = 0; i <= 2; i++) {
        List_first(ready_lists[i]);
        while (ready_lists[i]->current != NULL) {
            // Check for a match
            PCB *processPointer = ready_lists[i]->current->item;
            if (processPointer->pid == pid)
                return processPointer;
            // If no match, advance
            ready_lists[i]->current = ready_lists[i]->current->next;
        }
        // printf("Match not found in ready list %i...\n", i);  // Testing
    }

    // Search the waiting lists
    for (int i = 0; i <= 1; i++) {
        List_first(waiting_lists[i]);
        while (waiting_lists[i]->current != NULL) {
            // Check for a match
            PCB *processPointer = waiting_lists[i]->current->item;
            if (processPointer->pid == pid)
                return processPointer;
            // If no match, advance
            waiting_lists[i]->current = waiting_lists[i]->current->next;
        }
        // printf("Match not found in waiting list %i...\n", i);    // Testing
    }

    // Search the semaphore waiting lists
    for(int i = 0; i <= 4; i++) {
            if(sem_array[i].sem_init == true) {
                List_first(sem_array[i].pList);
                while (sem_array[i].pList->current != NULL) {
                    // Check for a match
                    PCB *processPointer = sem_array[i].pList->current->item;
                    if (processPointer->pid == pid)
                        return processPointer;
                    // If no match, advance
                    sem_array[i].pList->current = sem_array[i].pList->current->next;
            }
        }
    }

    // If we reach this line, process with the given pid was not found
    return NULL;
}

// Helper function to print process information to the screen
static void procinfo_helper(PCB *process) {

    printf("    Process ID:         %i\n", process->pid);
    printf("    Process Priority:   %i\n", process->priority);
    printf("    Process State:      ");
    if (process->state == RUNNING) {
        printf("RUNNING\n");
    } else if (process->state == READY) {
        printf("READY\n");
    } else {
        printf("BLOCKED\n");
    }

    // Only print these sections if not null
    if (process->proc_message != NULL) {
        printf("    Process Message:    %s", process->proc_message);
    }
    if (process->reply_msg != NULL) {
        printf("    Reply Message:      %s", process->reply_msg);
    }
    
    printf("\n");
}

static bool readyListEmpty() {
    for(int i = 0; i < 3; i++) {
        if(List_count(ready_lists[i]) != 0){
            return false;
        }
    }
    return true;
}

static void exit_sim() {
    freeProcess(INIT);
    exit_loop = true;
    return;
}