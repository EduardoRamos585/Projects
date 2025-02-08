// AssemblyFunct function
// Eduardo Ramos

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef ASSEMBLYFUNCT_H_
#define ASSEMBLYFUNCT_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

extern void setPrivMode(void);
extern void POPLR(void);
extern uint32_t getMsp(void);
extern uint32_t getPsp(void);
extern void setAspBit(void);
extern void setPSP(uint32_t* p);
extern uint32_t * InitStack(uint32_t * SP, uint32_t * fn);
extern uint32_t RunStack(void * SP);
extern void popMLr(void);
extern uint32_t* pushRegs();
extern uint32_t * popRegs();
extern uint32_t getR0();
extern void startRTOS();
extern void restartTask();
extern void stopTask();
extern void setTaskPriority();
extern void yieldTask();
extern void sleepTask();
extern void lockMutex();
extern void unlockMutex();
extern void waitSemaphore();
extern void postSemaphore();
extern void taskMalloc();
extern void shellPreempt();
extern void shellSched();
extern void shellReboot();
extern void shellPidof();
extern void shellKill();
extern void shellIpcs();
extern void shellMeminfo();
extern void shellTaskRun();
extern void shellPs();

#endif
