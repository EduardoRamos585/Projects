// Kernel functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "mm.h"
#include "kernel.h"
#include "shell.h"

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

// semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, all memory freed
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by semaphore
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

// task
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks

// control
bool priorityScheduler = true;    // priority (true) or round-robin (false)
bool priorityInheritance = false; // priority inheritance for mutexes
bool preemption = true;          // preemption (true) or cooperative (false)

// tcb
#define NUM_PRIORITIES   16
struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread (add of task fn)
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    uint8_t priority;              // 0=highest
    uint8_t currentPriority;       // 0=highest (needed for pi)
    uint32_t ticks;                // ticks until sleep complete
    uint64_t srd;                  // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    uint8_t mutex;                 // index of the mutex in use or blocking the thread
    uint8_t semaphore;             // index of the semaphore that is blocking the thread
    uint64_t A;
    uint64_t B;
} tcb[MAX_TASKS];


#define START 22
#define SWITCH 10
#define SLEEP 12
#define LOCK 23
#define UNLOCK 24
#define WAIT 35
#define POST 36
#define MALLOC 45
#define STOPTHREAD 76
#define RESTARTHREAD 77
#define SETPRIORITY 79
#define SCHED 56
#define PREEMPT 57
#define RESET 59
#define PIDOF 84
#define KILL 69
#define IPCS 67
#define MEMINFO 88
#define SHELLTASK 100
#define PS 101

uint8_t * LFM = NULL;
bool ping = false;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);
    if (ok)
    {
        mutexes[mutex].lock = false;
        mutexes[mutex].lockedBy = 0;
    }
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// REQUIRED: initialize systick for 1ms system timer
void initRtos(void)
{
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }

    NVIC_ST_RELOAD_R = 40000;
    NVIC_ST_CURRENT_R = 40000;
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;
}

// REQUIRED: Implement prioritization to NUM_PRIORITIES
uint8_t rtosScheduler(void)
{


            static uint8_t task = 0xFF;
            static uint8_t runTasks[15] = {};



            uint8_t Priority = 56;
            uint8_t i;
            uint8_t current;
            bool ok;


            if(priorityScheduler)
            {
               ok = false;
               for(i = 0; i < taskCount; i++)
               {
                   if((tcb[i].priority < Priority) && (tcb[i].state == STATE_READY))
                   {
                       Priority = tcb[i].priority;
                   }
               }

               while(!ok)
               {
                   current = (runTasks[Priority] + 1) % taskCount;
                   for(i = 0; i < taskCount; i++)
                   {
                       if((tcb[current].priority == Priority) && (tcb[current].state == STATE_READY))
                       {
                           task = current;
                           runTasks[Priority] = current;
                           ok = true;
                           break;
                       }
                       if (current >= taskCount)
                       {
                           current = 0;
                       }
                       current++;
                   }

                   if(!ok)
                   {
                       ok = true;
                       task = runTasks[Priority];
                   }

               }
            }
            else
            {
                 ok = false;
                 while (!ok)
                 {
                     task++;
                     if (task >= MAX_TASKS)
                         task = 0;
                     ok = (tcb[task].state == STATE_READY);
                 }
            }






          return task;
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, set srd bits, setting PSP, ASP bit, call fn with fn add in R0
// fn set TMPL bit, and PC <= fn
void startRtos(void)
{
    setPSP((uint32_t*)0x20008000);
    setAspBit();
    setPrivMode();
    startRTOS();
}

// REQUIRED:
// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// set the srd bits based on the memory allocation
// initialize the created stack to make it appear the thread has run before
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}
            tcb[i].state = STATE_READY;
            tcb[i].pid = fn;

            void * temp = mallocFromHeap(stackBytes);
            uint64_t Mask = createNoSramAccessMask();
            addSramAccessWindow(&Mask,(uint32_t *)temp,stackBytes);
            temp = (void *)((uint32_t)temp + stackBytes);
            tcb[i].sp = temp;
            tcb[i].spInit = temp;
            stringCopy(tcb[i].name,name);
            tcb[i].priority = priority;
            tcb[i].srd = Mask;


            //Set the stack as if it has run before

           tcb[i].sp =  (void *) InitStack((uint32_t *)tcb[i].sp, (uint32_t*)tcb[i].pid);


            // increment task count
            taskCount++;
            ok = true;
        }
    }
    return ok;
}

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
{
    restartTask();
}

// REQUIRED: modify this function to stop a thread
// REQUIRED: remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
    stopTask();
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
    setTaskPriority();
}

// REQUIRED: modify this function to yield execution back to scheduler using pendsv
void yield(void)
{
    yieldTask();  // Call serivce call to call Pend Sv isr
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    sleepTask();
}

// REQUIRED: modify this function to lock a mutex using pendsv
void lock(int8_t mutex)
{
    lockMutex();
}

// REQUIRED: modify this function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
    unlockMutex();
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    waitSemaphore();
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    postSemaphore();
}


// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr(void)
{
    uint8_t i;
    static uint16_t Count = 0;
    Count++;

    for(i = 0; i < MAX_TASKS; i++)
    {
        if(tcb[i].state == STATE_DELAYED)
        {
            tcb[i].ticks--;

            if(tcb[i].ticks == 0)
            {
               tcb[i].state = STATE_READY;

            }
        }
    }

    if(preemption)
    {
       NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
    }

    if(Count > 500)
    {
        ping ^= 1;

        if(ping)
        {
            tcb[taskCurrent].A = 0;
        }
        else
        {
            tcb[taskCurrent].B = 0;
        }

        Count = 0;
    }
}

// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
__attribute__ ((naked)) void pendSvIsr(void)
{
    //save Registers 4 - 11 and Exec Value




    if(ping)
    {
        tcb[taskCurrent].B += WTIMER0_TAV_R;
    }
    else
    {
        tcb[taskCurrent].A += WTIMER0_TAV_R;
    }

    if(NVIC_FAULT_STAT_R & (NVIC_FAULT_STAT_DERR | NVIC_FAULT_STAT_IERR))
       {
          putsUart0("\ncalled from MPU");
          tcb[taskCurrent].state = STATE_STOPPED;
          uint32_t * SP = (uint32_t *)tcb[taskCurrent].spInit;
          if((SP > (uint32_t*)0x20001000) && (SP < (uint32_t*)0x20002000))
          {
               //Block is 4k (one section is 512)
               SP = (uint32_t *) ((uint32_t) SP - (uint32_t)512);
               freeToHeap((void*)SP);
          }
          else if((SP > (uint32_t*) 0x20002000) && (SP < (uint32_t*) 0x20003C00))
          {
               //Block is 8k (1024 is one section)
               SP = (uint32_t*) ((uint32_t) SP - (uint32_t)1024);
               freeToHeap((void*)SP);

          }
          else if((SP > (uint32_t*)0x20003C00) && (SP < (uint32_t*)0x20004200))
          {
               //Block is in special region (1536)
               SP = (uint32_t *) ((uint32_t) SP - (uint32_t)1536);
               freeToHeap((void*)SP);

          }
          else if((SP > (uint32_t*) 0x20004200) && (SP < (uint32_t*)0x20006000))
          {
               //Block is the two 4k regions (512)
               SP = (uint32_t*) ((uint32_t) SP - (uint32_t)512);
               freeToHeap((void*)SP);

          }
          else if((SP > (uint32_t*)0x20006000) && (SP < (uint32_t*)0x20008000))
          {
               //Block is 8K region (1024)
               SP = (uint32_t*)((uint32_t) SP - (uint32_t)1024);
               freeToHeap((void*)SP);
          }
          NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_IERR|NVIC_FAULT_STAT_UNDEF;
          tcb[taskCurrent].srd = createNoSramAccessMask();
          taskCurrent= rtosScheduler();
          applySramAccessMask(tcb[taskCurrent].srd);
          setPSP((uint32_t *) tcb[taskCurrent].sp);
          tcb[taskCurrent].sp = (void *) popRegs();
          POPLR();

       }
       else
       {
           tcb[taskCurrent].sp = (void *) pushRegs();
           taskCurrent= rtosScheduler();
           WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;

           WTIMER0_TAV_R = 0;
           WTIMER0_CTL_R |= TIMER_CTL_TAEN;

           applySramAccessMask(tcb[taskCurrent].srd);
           setPSP((uint32_t *) tcb[taskCurrent].sp);
           tcb[taskCurrent].sp = (void *) popRegs();
           POPLR();
       }






}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void)
{
    //Get svc num


    uint32_t* P = (uint32_t *) getPsp();
    uint16_t * PC = (uint16_t *) *(P + 6); // We need to move PC back by 2 bytes
    PC -= 1;
    uint16_t svcCall = (uint16_t)*PC;
    uint8_t Num;
    Num = svcCall & 0xFF;
    uint32_t * Temp = NULL;
    uint32_t ticks;
    int8_t mutex;
    int8_t semaphore;
    uint32_t Bytes;
    uint64_t newMask;
    void * Mall = NULL;
    void * PID = NULL;
    char * String = NULL;
    bool option = false;
    uint32_t * SP;
    uint8_t i;
    uint8_t j;
    uint8_t index;
    uint8_t priority;
    uint32_t ID = 0;
    char sent[15] = {};
    uint64_t CPU = 0;
    uint64_t SUM = 0;




    switch(Num)
    {
      case START:{
         taskCurrent= rtosScheduler();
         applySramAccessMask(tcb[taskCurrent].srd);
         Temp = (uint32_t *)RunStack(tcb[taskCurrent].sp);
         tcb[taskCurrent].sp = (void *)Temp;
         setPSP(Temp);
         NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_ENABLE | NVIC_MPU_CTRL_PRIVDEFEN;
         POPLR();
         break;
      }

      case SWITCH:{
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
        break;
      }

      case SLEEP:{
      // We must obtain the nummber of tikcs from R0 (PSP stack)
          ticks = *((uint32_t *)getPsp());
          tcb[taskCurrent].ticks = ticks;
          tcb[taskCurrent].state = STATE_DELAYED;
          NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
          break;
      }

      case LOCK:{
          mutex = (int8_t)*((uint32_t *)getPsp());
          if((mutexes[mutex].lock == false))
          {
              tcb[taskCurrent].mutex = mutex;
              mutexes[mutex].lock = true;
              mutexes[mutex].lockedBy = taskCurrent;
          }
          else
          {
             tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;
             tcb[taskCurrent].mutex = mutex;
             mutexes[mutex].processQueue[mutexes[mutex].queueSize] = taskCurrent;
             mutexes[mutex].queueSize++;
             NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;

          }
          break;
      }

      case UNLOCK:{
              mutex = (int8_t)*((uint32_t *)getPsp());

              if(mutexes[mutex].lockedBy == taskCurrent)
              {

                  if(mutexes[mutex].queueSize > 0)
                  {
                      tcb[mutexes[mutex].processQueue[0]].state = STATE_READY;
                      tcb[mutexes[mutex].processQueue[0]].mutex = mutex;
                      mutexes[mutex].lockedBy = mutexes[mutex].processQueue[0];
                      mutexes[mutex].queueSize--;
                      uint8_t i;
                      for(i = 0; i < (mutexes[mutex].queueSize); i++)
                      {
                           mutexes[mutex].processQueue[i] = mutexes[mutex].processQueue[i+1];
                      }

                  }
                  else
                  {
                      mutexes[mutex].lock = false;

                  }
              }
              else
              {
                  tcb[taskCurrent].state = STATE_STOPPED;
              }
              NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
          break;
      }

      case WAIT:{
          semaphore = (int8_t)*((uint32_t *)getPsp());
          if(semaphores[semaphore].count > 0)
          {
              semaphores[semaphore].count--;
              return;
          }
          else
          {
              semaphores[semaphore].processQueue[semaphores[semaphore].queueSize] = taskCurrent;
              semaphores[semaphore].queueSize++;
              tcb[taskCurrent].semaphore = semaphore;
              tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;
              NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
          }
          break;
      }

      case POST:{
           semaphore = (int8_t)*((uint32_t *)getPsp());
           semaphores[semaphore].count++;

           if((semaphores[semaphore].queueSize > 0))
           {
               tcb[semaphores[semaphore].processQueue[0]].state = STATE_READY;
               tcb[semaphores[semaphore].processQueue[0]].semaphore = semaphore;
               semaphores[semaphore].queueSize--;
               semaphores[semaphore].count--;
               semaphores[semaphore].processQueue[0] = semaphores[semaphore].processQueue[1];
           }
           break;
      }
      case MALLOC:{
              Bytes = *((uint32_t*)getPsp());
              Mall = mallocFromHeap(Bytes);
              newMask = createNoSramAccessMask();
              addSramAccessWindow(&newMask,(uint32_t *)Mall,Bytes);
              tcb[taskCurrent].srd &= newMask;
              applySramAccessMask(tcb[taskCurrent].srd);
              LFM = (uint8_t *)Mall;
              *((uint32_t *)getPsp()) = (uint32_t)Mall;
          break;
      }

      case STOPTHREAD:{
               PID = (void *) *((uint32_t *)getPsp());
               for(i = 0; i < MAX_TASKS; i++)
               {
                  if(tcb[i].pid == PID)
                  {
                      index = i;
                  }
               }

              if(tcb[index].state == STATE_BLOCKED_MUTEX)
              {
                  if(mutexes[tcb[index].mutex].lockedBy == index)
                  {
                      mutexes[tcb[index].mutex].lock = false;
                      mutexes[tcb[index].mutex].lockedBy = 0;

                  }
                  else
                  {
                      //Look for thread in line(remove it)
                      if(mutexes[tcb[index].mutex].processQueue[0] == index)
                      {
                          mutexes[tcb[index].mutex].processQueue[0] = mutexes[tcb[index].mutex].processQueue[1];
                      }
                      else if(mutexes[tcb[index].mutex].processQueue[1] == index)
                      {
                          mutexes[tcb[index].mutex].processQueue[1] = 0;
                      }
                  }

              }
              else if(tcb[index].state == STATE_BLOCKED_SEMAPHORE)
              {
                  //Look for the thread in the queue(remove it if found)
                  if(semaphores[tcb[index].semaphore].processQueue[0] == index)
                  {
                     semaphores[tcb[index].semaphore].processQueue[0] = semaphores[tcb[index].semaphore].processQueue[1];
                  }
                  else if(semaphores[tcb[index].semaphore].processQueue[1] == index)
                  {
                     semaphores[tcb[index].semaphore].processQueue[1] = 0;
                  }
              }

              tcb[index].state = STATE_STOPPED;
              //We need to free memory

               SP = (uint32_t *)tcb[index].spInit;
              //Find the block the stack pointer is on

               if(stringCompare(tcb[index].name, "LengthyFn"))
               {
                     //We know the allocation of LengthyFn and it's inner malloc

                     SP = (uint32_t *)((uint32_t)SP - 1024);
                     freeToHeap((void*)SP); // Free Lengthyfn
                     freeToHeap((void *)LFM);

              }
              else
              {
                   if((SP > (uint32_t*)0x20001000) && (SP <= (uint32_t*)0x20002000))
                   {
                       //Block is 4k (one section is 512)
                       SP = (uint32_t *) ((uint32_t) SP - (uint32_t)512);
                       freeToHeap((void*)SP);
                   }
                   else if((SP > (uint32_t*) 0x20002000) && (SP <= (uint32_t*) 0x20003C00))
                   {
                       //Block is 8k (1024 is one section)
                       SP = (uint32_t*) ((uint32_t) SP - (uint32_t)1024);
                       freeToHeap((void*)SP);

                   }
                   else if((SP > (uint32_t*)0x20003C00) && (SP <= (uint32_t*)0x20004200))
                   {
                       //Block is in special region (1536)
                       SP = (uint32_t *) ((uint32_t) SP - (uint32_t)1536);
                       freeToHeap((void*)SP);

                   }
                   else if((SP > (uint32_t*) 0x20004200) && (SP <= (uint32_t*)0x20006000))
                   {
                       //Block is the two 4k regions (512)
                       SP = (uint32_t*) ((uint32_t) SP - (uint32_t)512);
                       freeToHeap((void*)SP);

                   }
                   else if((SP > (uint32_t*)0x20006000) && (SP <= (uint32_t*)0x20008000))
                   {
                       if(stringCompare(tcb[index].name,"Shell"))
                       {
                           SP = (uint32_t*)((uint32_t) SP - (uint32_t)4096);
                           freeToHeap((void*)SP);
                       }
                       else
                       {
                           //Block is 8K region (1024)
                           SP = (uint32_t*)((uint32_t) SP - (uint32_t)1024);
                           freeToHeap((void*)SP);
                       }

                   }

             }

          break;
      }

      case RESTARTHREAD:{
           PID = (void *) *((uint32_t *)getPsp());

           for(i = 0; i < MAX_TASKS; i++)
           {
               if(tcb[i].pid == PID)
               {
                   index = i;
               }
           }

           tcb[index].state = STATE_READY;
           SP = (uint32_t *)tcb[index].spInit;
           //Find the block the stack pointer is on

           if((SP > (uint32_t*)0x20001000) && (SP <= (uint32_t*)0x20002000))
           {
               Mall = mallocFromHeap(512);
               newMask = createNoSramAccessMask();
               addSramAccessWindow(&newMask,(uint32_t *)Mall,(uint32_t)512);
               Mall = (void *)((uint32_t)Mall + 512);
               tcb[index].sp = Mall;
               tcb[index].spInit = Mall;
               tcb[index].srd = newMask;
               //Set the stack as if it has run before

              tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);
           }
           else if((SP > (uint32_t*) 0x20002000) && (SP <= (uint32_t*) 0x20003C00))
           {
               //Block is 8k (1024 is one section)
               Mall = mallocFromHeap(1024);
               newMask = createNoSramAccessMask();
               addSramAccessWindow(&newMask,(uint32_t *)Mall,1024);
               Mall = (void *)((uint32_t)Mall + 1024);
               tcb[index].sp = Mall;
               tcb[index].spInit = Mall;
               tcb[index].srd = newMask;
               //Set the stack as if it has run before

               tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);

           }
           else if((SP > (uint32_t*)0x20003C00) && (SP <= (uint32_t*)0x20004200))
           {
               //Block is in special region (1536)
               Mall = mallocFromHeap(1536);
               newMask = createNoSramAccessMask();
               addSramAccessWindow(&newMask,(uint32_t *)Mall,1536);
               Mall = (void *)((uint32_t)Mall + 1536);
               tcb[index].sp = Mall;
               tcb[index].spInit = Mall;
               tcb[index].srd = newMask;
               //Set the stack as if it has run before

               tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);

           }
           else if((SP > (uint32_t*) 0x20004200) && (SP <= (uint32_t*)0x20006000))
           {
               //Block is the two 4k regions (512)
               Mall = mallocFromHeap(512);
               newMask = createNoSramAccessMask();
               addSramAccessWindow(&newMask,(uint32_t *)Mall,512);
               Mall = (void *)((uint32_t)Mall + 512);
               tcb[index].sp = Mall;
               tcb[index].spInit = Mall;
               tcb[index].srd = newMask;
               //Set the stack as if it has run before

               tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);

           }
           else if((SP > (uint32_t*)0x20006000) && (SP <= (uint32_t*)0x20008000))
           {
               if(stringCompare(tcb[index].name,"Shell"))
               {
                   //Block is 8K region (1024)
                  Mall = mallocFromHeap(4096);
                  newMask = createNoSramAccessMask();
                  addSramAccessWindow(&newMask,(uint32_t *)Mall,4096);
                  Mall = (void *)((uint32_t)Mall + 4096);
                  tcb[index].sp = Mall;
                  tcb[index].spInit = Mall;
                  tcb[index].srd = newMask;
                  //Set the stack as if it has run before

                  tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);
               }
               else
               {
                  //Block is 8K region (1024)
                  Mall = mallocFromHeap(1024);
                  newMask = createNoSramAccessMask();
                  addSramAccessWindow(&newMask,(uint32_t *)Mall,1024);
                  Mall = (void *)((uint32_t)Mall + 1024);
                  tcb[index].sp = Mall;
                  tcb[index].spInit = Mall;
                  tcb[index].srd = newMask;
                  //Set the stack as if it has run before

                  tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);
               }


           }


          break;
      }

      case SETPRIORITY:{
              PID = (void *) *((uint32_t *)getPsp());
              priority = (uint8_t) *((uint32_t *)(getPsp()) + 1);
              for(i = 0; i < MAX_TASKS; i++)
              {
                  if(tcb[i].pid == PID)
                  {
                      index = i;
                  }
              }

              tcb[index].priority = priority;

          break;
      }
      case SCHED:{
          option = (bool) *((uint32_t *)getPsp());
          if(option)
          {
            priorityScheduler = true;
            putsUart0("Scheduler : Priority\n");
          }
          else
          {
              priorityScheduler = false;
              putsUart0("Scheduler : Round Robin \n");
          }
          break;

      }
      case PREEMPT:{
          option = (bool) *((uint32_t *)getPsp());
          if(option)
          {
              preemption = true;
              putsUart0("Preemption Active\n");
          }
          else
          {
              preemption = false;
              putsUart0("Preemption Disabled\n");
          }
          break;

      }
      case RESET:{
          NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
      }

      case PIDOF:{
          String = (char*)*((uint32_t *)getPsp() + 8);
          for(i = 0; i < MAX_TASKS; i++)
          {
              if(stringCompare(String, tcb[i].name))
              {
                  index = i;
              }

          }

          *((uint32_t *)getPsp()) = (uint32_t)tcb[index].pid;
          break;
      }
      case KILL:{
          ID = (uint32_t)*((uint32_t *)getPsp());
          for(i = 0; i < MAX_TASKS; i++)
          {
              if(((uint32_t)tcb[i].pid) == ID)
               {
                  index = i;
               }
          }

          SP = (uint32_t*)tcb[index].spInit;
          if((tcb[index].state == STATE_BLOCKED_MUTEX) || (mutexes[tcb[index].mutex].lockedBy == index))
          {
              if(mutexes[tcb[index].mutex].lockedBy == index)
              {

                  if(mutexes[tcb[index].mutex].queueSize > 0)
                  {
                    tcb[mutexes[tcb[index].mutex].processQueue[0]].state = STATE_READY;
                    tcb[mutexes[tcb[index].mutex].processQueue[0]].mutex = tcb[index].mutex;
                    mutexes[tcb[index].mutex].lockedBy = mutexes[tcb[index].mutex].processQueue[0];
                    mutexes[tcb[index].mutex].queueSize--;
                    mutexes[tcb[index].mutex].processQueue[0] = mutexes[tcb[index].mutex].processQueue[1];

                  }
                  else
                  {
                      mutexes[tcb[index].mutex].lock = false;

                  }

              }
              else
              {
                    //Look for thread in line(remove it)
                 if(mutexes[tcb[index].mutex].processQueue[0] == index)
                 {
                     mutexes[tcb[index].mutex].processQueue[0] = mutexes[tcb[index].mutex].processQueue[1];
                 }
                 else if(mutexes[tcb[index].mutex].processQueue[1] == index)
                 {
                     mutexes[tcb[index].mutex].processQueue[1] = 0;
                 }
                 mutexes[tcb[index].mutex].queueSize--;
              }


          }

          if(tcb[index].state == STATE_BLOCKED_SEMAPHORE)
          {
              //Look for the thread in the queue(remove it if found)
              if(semaphores[tcb[index].semaphore].processQueue[0] == index)
              {
                 semaphores[tcb[index].semaphore].processQueue[0] = semaphores[tcb[index].semaphore].processQueue[1];
              }
              else if(semaphores[tcb[index].semaphore].processQueue[1] == index)
              {
                 semaphores[tcb[index].semaphore].processQueue[1] = 0;
              }

          }
          tcb[index].state = STATE_STOPPED;
          tcb[index].srd = createNoSramAccessMask();
          if(stringCompare(tcb[index].name, "LengthyFn"))
          {
              //We know the allocation of LengthyFn and it's inner malloc

              SP = (uint32_t *)((uint32_t)SP - 1024);
              freeToHeap((void*)SP); // Free Lengthyfn
              freeToHeap((void *)LFM);

          }
          else
          {
              if((SP > (uint32_t*)0x20001000) && (SP <= (uint32_t*)0x20002000))
              {
                    //Block is 4k (one section is 512)
                    SP = (uint32_t *) ((uint32_t) SP - (uint32_t)512);
                    freeToHeap((void*)SP);
              }
              else if((SP > (uint32_t*) 0x20002000) && (SP <= (uint32_t*) 0x20003C00))
              {
                    //Block is 8k (1024 is one section)
                    SP = (uint32_t*) ((uint32_t) SP - (uint32_t)1024);
                    freeToHeap((void*)SP);

              }
              else if((SP > (uint32_t*)0x20003C00) && (SP <= (uint32_t*)0x20004200))
              {
                    //Block is in special region (1536)
                    SP = (uint32_t *) ((uint32_t) SP - (uint32_t)1536);
                    freeToHeap((void*)SP);

              }
              else if((SP > (uint32_t*) 0x20004200) && (SP <= (uint32_t*)0x20006000))
              {
                    //Block is the two 4k regions (512)
                    SP = (uint32_t*) ((uint32_t) SP - (uint32_t)512);
                    freeToHeap((void*)SP);

              }
              else if((SP > (uint32_t*)0x20006000) && (SP <= (uint32_t*)0x20008000))
              {
                    if(stringCompare(tcb[index].name,"Shell"))
                    {
                        SP = (uint32_t*)((uint32_t) SP - (uint32_t)4096);
                        freeToHeap((void*)SP);
                    }
                    else
                    {
                        //Block is 8K region (1024)
                        SP = (uint32_t*)((uint32_t) SP - (uint32_t)1024);
                        freeToHeap((void*)SP);
                    }

              }

          }

          break;

      }
      case IPCS:{

          putsUart0("\tPrinting Mutex and Semaphore information\n");
          putsUart0("Mutex         Tasks in Queue            State\n");
          putsUart0("Resource");
          for(i = 0; i < mutexes[0].queueSize; i++)
          {
              putsUart0("   \t");
              putsUart0(tcb[mutexes[0].processQueue[i]].name);
          }
          if(mutexes[0].lock == true)
          {
              putsUart0(" \t\tLocked\n");
          }
          else
          {
              putsUart0(" \t\tUnlocked\n");
          }
          putsUart0("Currently locked by: ");
          putsUart0(tcb[mutexes[0].lockedBy].name);
          putsUart0("\n");
          putsUart0("\n");
          putsUart0("\t\tSemaphore Section\n");
          putsUart0("Semaphore          Tasks in Queue            Count\n");
          putsUart0("keyPressed");
          for(i = 0; i < semaphores[0].queueSize; i++)
          {
              putsUart0("           ");
              putsUart0(tcb[semaphores[0].processQueue[i]].name);
          }
          putsUart0("       \t\t");
          Bytes = (uint32_t)semaphores[0].count;
          numToString(Bytes,sent,false,false);
          putsUart0(sent);
          putsUart0("\n");
          putsUart0("keyReleased");
          if(semaphores[1].queueSize == 0)
          {
              putsUart0("        No tasks in queue");
          }
          else
          {
               for(i = 0; i < semaphores[1].queueSize; i++)
               {
                    putsUart0("           ");
                    putsUart0(tcb[semaphores[1].processQueue[i]].name);
               }
          }

          putsUart0("   \t\t");
          Bytes = (uint32_t)semaphores[1].count;
          numToString(Bytes,sent,false,false);
          putsUart0(sent);
          putsUart0("\n");
          putsUart0("flashReq");
          if(semaphores[2].queueSize == 0)
          {
               putsUart0("           No tasks in queue");
               putsUart0("  \t\t");
          }
          else
          {
              for(i = 0; i < semaphores[2].queueSize; i++)
              {
                   putsUart0("             ");
                   putsUart0(tcb[semaphores[2].processQueue[i]].name);
              }
              putsUart0("      \t\t");

          }
          Bytes = (uint32_t)semaphores[2].count;
          numToString(Bytes,sent,false,false);
          putsUart0(sent);
          break;

      }
      case MEMINFO:{

          putsUart0("\t\t       Stack Allocation         \t\t\n");

          for(i = 0; i < taskCount; i++)
          {
              putsUart0(tcb[i].name);
              putsUart0("          \t\t\t");
              if((i == 0) || (i == 9))
              {
                 putsUart0("  \t");
              }
              SP = (uint32_t *)tcb[i].spInit;

              if((SP > (uint32_t*)0x20001000) && (SP <= (uint32_t*)0x20002000))
              {
                      //Block is 4k (one section is 512)
                      SP = (uint32_t *) ((uint32_t) SP - (uint32_t)512);
                      numToString((uint32_t)SP,sent,false,true);
                      putsUart0(sent);
                      putsUart0("\n");
                      putsUart0("Size:             \t\t\t512\n");
                      putsUart0("--------------------------------------------------------------------\n");

              }
              else if((SP > (uint32_t*) 0x20002000) && (SP <= (uint32_t*) 0x20003C00))
              {
                      //Block is 8k (1024 is one section)
                      if((stringCompare(tcb[i].name,"LengthyFn")))
                      {
                        SP = (uint32_t*) ((uint32_t) SP - (uint32_t)1024);
                        numToString((uint32_t)SP,sent,false,true);
                        putsUart0(sent);
                        putsUart0("\n");
                        putsUart0("Size:             \t\t\t1024\n\n");
                        putsUart0("Additonal Malloc:");
                        putsUart0("   \t\t\t");
                        numToString((uint32_t)LFM,sent,false,true);
                        putsUart0(sent);
                        putsUart0("\n");
                        putsUart0("Size:            \t\t\t5000\n");
                        putsUart0("--------------------------------------------------------------------\n");
                      }
                      else
                      {
                         SP = (uint32_t*) ((uint32_t) SP - (uint32_t)1024);
                         numToString((uint32_t)SP,sent,false,true);
                         putsUart0(sent);
                         putsUart0("\n");
                         putsUart0("Size:             \t\t\t1024\n");
                         putsUart0("--------------------------------------------------------------------\n");

                      }



              }
              else if((SP > (uint32_t*)0x20003C00) && (SP <= (uint32_t*)0x20004200))
              {
                      //Block is in special region (1536)
                      SP = (uint32_t *) ((uint32_t) SP - (uint32_t)1536);
                      numToString((uint32_t)SP,sent,false,true);
                      putsUart0(sent);
                      putsUart0("\n");
                      putsUart0("Size:              \t\t\t1536\n");
                      putsUart0("--------------------------------------------------------------------\n");



              }
              else if((SP > (uint32_t*) 0x20004200) && (SP <= (uint32_t*)0x20006000))
              {
                      //Block is the two 4k regions (512)
                      SP = (uint32_t*) ((uint32_t) SP - (uint32_t)512);
                      numToString((uint32_t)SP,sent,false,true);
                      putsUart0(sent);
                      putsUart0("\n");
                      putsUart0("Size:              \t\t\t512\n");
                      putsUart0("--------------------------------------------------------------------\n");



              }
              else if((SP > (uint32_t*)0x20006000) && (SP <= (uint32_t*)0x20008000))
              {
                      //Block is 8K region (1024)
                      if((stringCompare(tcb[i].name, "Shell")))
                      {
                         SP = (uint32_t*)((uint32_t) SP - (uint32_t)4096);
                         numToString((uint32_t)SP,sent,false,true);
                         putsUart0(sent);
                         putsUart0("\n");
                         putsUart0("Size:            \t\t\t4096\n");
                         putsUart0("--------------------------------------------------------------------\n");

                      }
                      else
                      {
                        SP = (uint32_t*)((uint32_t) SP - (uint32_t)1024);
                        numToString((uint32_t)SP,sent,false,true);
                        putsUart0(sent);
                        putsUart0("\n");
                        putsUart0("Size:            \t\t\t1024\n");
                        putsUart0("--------------------------------------------------------------------\n");

                      }

              }
          }

          break;

      }
      case SHELLTASK: {
          String = (char*)*((uint32_t *)getPsp() + 8);
          index = 56;
          for(i = 0; i < MAX_TASKS; i++)
          {
                if(stringCompare(String, tcb[i].name))
                {
                    index = i;
                }

          }

          if(index == 56)
          {
              putsUart0("Command unrecognized\n");
          }
          else
          {
              tcb[index].state = STATE_READY;
              SP = (uint32_t *)tcb[index].spInit;
              //Find the block the stack pointer is on

              if((SP > (uint32_t*)0x20001000) && (SP <= (uint32_t*)0x20002000))
              {
                 Mall = mallocFromHeap(512);
                 newMask = createNoSramAccessMask();
                 addSramAccessWindow(&newMask,(uint32_t *)Mall,(uint32_t)512);
                 Mall = (void *)((uint32_t)Mall + 512);
                 tcb[index].sp = Mall;
                 tcb[index].spInit = Mall;
                 tcb[index].srd = newMask;
                 //Set the stack as if it has run before

                tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);
              }
              else if((SP > (uint32_t*) 0x20002000) && (SP <= (uint32_t*) 0x20003C00))
              {
                 //Block is 8k (1024 is one section)
                 Mall = mallocFromHeap(1024);
                 newMask = createNoSramAccessMask();
                 addSramAccessWindow(&newMask,(uint32_t *)Mall,1024);
                 Mall = (void *)((uint32_t)Mall + 1024);
                 tcb[index].sp = Mall;
                 tcb[index].spInit = Mall;
                 tcb[index].srd = newMask;
                 //Set the stack as if it has run before

                 tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);

              }
              else if((SP > (uint32_t*)0x20003C00) && (SP <= (uint32_t*)0x20004200))
              {
                 //Block is in special region (1536)
                 Mall = mallocFromHeap(1536);
                 newMask = createNoSramAccessMask();
                 addSramAccessWindow(&newMask,(uint32_t *)Mall,1536);
                 Mall = (void *)((uint32_t)Mall + 1536);
                 tcb[index].sp = Mall;
                 tcb[index].spInit = Mall;
                 tcb[index].srd = newMask;
                 //Set the stack as if it has run before

                 tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);

              }
              else if((SP > (uint32_t*) 0x20004200) && (SP <= (uint32_t*)0x20006000))
              {
                 //Block is the two 4k regions (512)
                 Mall = mallocFromHeap(512);
                 newMask = createNoSramAccessMask();
                 addSramAccessWindow(&newMask,(uint32_t *)Mall,512);
                 Mall = (void *)((uint32_t)Mall + 512);
                 tcb[index].sp = Mall;
                 tcb[index].spInit = Mall;
                 tcb[index].srd = newMask;
                 //Set the stack as if it has run before

                 tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);

              }
              else if((SP > (uint32_t*)0x20006000) && (SP <= (uint32_t*)0x20008000))
              {
                 if(stringCompare(tcb[index].name,"Shell"))
                 {
                     //Block is 8K region (1024)
                    Mall = mallocFromHeap(4096);
                    newMask = createNoSramAccessMask();
                    addSramAccessWindow(&newMask,(uint32_t *)Mall,4096);
                    Mall = (void *)((uint32_t)Mall + 4096);
                    tcb[index].sp = Mall;
                    tcb[index].spInit = Mall;
                    tcb[index].srd = newMask;
                    //Set the stack as if it has run before

                    tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);
                 }
                 else
                 {
                    //Block is 8K region (1024)
                    Mall = mallocFromHeap(1024);
                    newMask = createNoSramAccessMask();
                    addSramAccessWindow(&newMask,(uint32_t *)Mall,1024);
                    Mall = (void *)((uint32_t)Mall + 1024);
                    tcb[index].sp = Mall;
                    tcb[index].spInit = Mall;
                    tcb[index].srd = newMask;
                    //Set the stack as if it has run before

                    tcb[index].sp =  (void *) InitStack((uint32_t *)tcb[index].sp, (uint32_t*)tcb[index].pid);
                 }


             }
         }

          break;

      }
      case PS:{



          putsUart0("\t\t Displaying Task Information \t\t\n");
          for(i = 0; i < taskCount; i++)
          {

              if(ping)
             {
                for(j = 0; j < taskCount; j++)
                {
                    SUM += tcb[j].B;
                }
                CPU = ((float)(tcb[i].B) / (SUM)) * 1000;
             }
             else
             {
                for(j = 0; j < taskCount; j++)
                {
                    SUM += tcb[j].A;
                }
                CPU = ((float)(tcb[i].A) / (SUM)) * 1000;

              }
              putsUart0("PID: ");
              numToString((uint32_t)tcb[i].pid,sent,true,false);
              putsUart0(sent);
              putsUart0("  Name:");
              putsUart0(tcb[i].name);

              if((i == 0)|| (i == 7) || (i == 8) || (i == 9))
              {
                    putsUart0("\t");
              }

              putsUart0(" STATE:");


              if(tcb[i].state == STATE_INVALID)
              {
                  putsUart0("INVALID");
              }
              else if(tcb[i].state == STATE_STOPPED)
              {
                  putsUart0("STOPPED");
              }
              else if(tcb[i].state == STATE_READY)
              {
                  putsUart0("READY");
              }
              else if(tcb[i].state == STATE_DELAYED)
              {
                  putsUart0("DELAYED");
              }
              else if(tcb[i].state == STATE_BLOCKED_MUTEX)
              {
                  putsUart0("BLOCKED");
              }
              else
              {
                  putsUart0("BLOCKED");
              }

              putsUart0("    CPU:");
              numToString((uint32_t)CPU,sent,false,false);
              putsUart0(sent);
              putsUart0(".");
              putsUart0("00%");

              if(tcb[i].state == STATE_BLOCKED_MUTEX)
              {
                  putsUart0("     Blocked by: Resource");
              }
              else if(tcb[i].state == STATE_BLOCKED_SEMAPHORE)
              {
                  if(tcb[i].semaphore == 0)
                  {
                      putsUart0("    Blocked by: keyPressed");
                  }
                  else if(tcb[i].semaphore == 1)
                  {
                      putsUart0("    Blocked by: keyReleased");
                  }
                  else if(tcb[i].semaphore == 2)
                  {
                      putsUart0("     Blocked by: flashRequest");
                  }
              }

              putsUart0("\n");



          }

          break;

      }


      default:
          break;
    }

    //Hardware will pop R0-3, R12, LR, PC, Xpsr
    //Software must pop R4-R11 & new LR value

}

uint8_t * mallocCall(uint32_t sizeBytes)
{
    taskMalloc();
    uint8_t * SP = (uint8_t *) getR0();
    return SP;
}











