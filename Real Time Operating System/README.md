## Description
The goal of this project was to replicate the operations of a Real-Time Operating System. For this project, the TI Tiva C Series board was used.

Of course, there were some limitations to this design, especially when it came to multithreading. Since the Tiva TM4C123GH6PM comes with only one ARM Cortex-M4F core, running tasks simultaneously was not an option. Therefore, context switching was necessary for this design.

## Functionality 
In a RTOS, the indivdual characteristics of the design ensure the following functionalities:

Task Managment:
* Each task is allocated a fixed time slice for execution, with higher-priority tasks scheduled before lower-priority ones.
* Each task transitions between different states (e.g., Ready, Running, Blocked) that determine whether it is eligible for execution by the scheduler.

```c
// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, all memory freed
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by semaphore
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore
``` 

--------------------------------------------------------------------------
Real-Time Scheduling:
* To minimize task latency, the RTOS may support switching between various scheduling algorithms (e.g., preemptive, cooperative) based on system requirements.
* Round robin scheduling allows for all tasks to run regardless of priority.
* Priority scheduling seelcts the highest priority tasks to run first.

```c
uint8_t rtosScheduler(void)
{
            static uint8_t task = 0xFF;
            static uint8_t runTasks[15] = {};
            uint8_t Priority = 56;
            uint8_t i;
            uint8_t current;
            bool ok;


            if(priorityScheduler){
               ok = false;
               for(i = 0; i < taskCount; i++){
                   if((tcb[i].priority < Priority) && (tcb[i].state == STATE_READY)){
                       Priority = tcb[i].priority;
                   }
               }
               while(!ok){
                   current = (runTasks[Priority] + 1) % taskCount;
                   for(i = 0; i < taskCount; i++){
                       if((tcb[current].priority == Priority) && (tcb[current].state == STATE_READY)){
                           task = current;
                           runTasks[Priority] = current;
                           ok = true;
                           break;
                       }
                       if (current >= taskCount){
                           current = 0;
                       }
                       current++;
                   }

                   if(!ok){
                       ok = true;
                       task = runTasks[Priority];
                   }
               }
            }
            else{
                 ok = false;
                 while (!ok){
                     task++;
                     if (task >= MAX_TASKS)
                         task = 0;
                     ok = (tcb[task].state == STATE_READY);
                 }
            }
            return task;
}
```



 ----------------------------------------------------------------------------

  

Inter-task Communication
* Synchronization and resource sharing between tasks are achieved through data-sharing mechanisms such as semaphores and mutexes.
* Mutexes prevent multiple tasks from accessing a critical region.
* Sempahores allow for tasks to wait/use shared resources.
Mutexes are created so:
```c
 typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];
```
While Semaphores are constructed as such:

```c
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

```
-------------------------------------------------------------------------------
## How it works:
 The OS starts by creating 10 tasks, each with a different priority level. To ensure they have the necessary space for storing variables, memory is allocated to each task. Once created, the microcontroller switches to unprivileged mode, allowing each task to run independently.

After a certain amount of time—especially when preemptive scheduling is enabled—a task may yield the program counter to another task. In such cases, the contents of the stack must be preserved. To accomplish this, we use the following function:
  ```assembly
        pushRegs:
			MRS R0, PSP
			SUB R0, #4
			STR R11, [R0], #-4
			STR R10, [R0], #-4
			STR R9, [R0], #-4
			STR R8, [R0], #-4
			STR R7, [R0], #-4
			STR R6, [R0], #-4
			STR R5, [R0], #-4
			STR R4, [R0]
			BX LR 
  ```
By pushing the contents of a task's stack, we ensure that no data is lost before a context switch. When switching to a new task, the stack is restored by reversing the process used to save it.

However, since programming isn't perfect, there is always a possibility that a task may malfunction and compromise the stability of the OS. Fortunately, the TM4C123GH6PM microcontroller includes a Memory Protection Unit (MPU), which helps prevent unprivileged tasks from accessing protected OS memory space.

If such a violation occurs, the OS automatically terminates the offending task to prevent further damage to the system.

<img src="Images/Am4 Core .png" alt="main" width="400"/>

------------------------------------------------------------------------------



  
  
  
