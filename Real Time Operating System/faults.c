// Shell functions
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
#include "faults.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: If these were written in assembly
//           omit this file and add a faults.s file

// REQUIRED: code this function
void mpuFaultIsr(void)
{
    putsUart0("\nMPU fault has occurred \n");

    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMP;

    char Line[12];


    uint32_t stackMp = getMsp();
    uint32_t stackP = getPsp();
    uint32_t *psp = (uint32_t*)stackP;
    uint32_t R0 = *psp;
    uint32_t R1 = *(psp+1);
    uint32_t R2 = *(psp+2);
    uint32_t R3 = *(psp+3);

    uint32_t R12 = *(psp + 4);

    uint32_t LR = *(psp + 5);
    uint32_t PC = *(psp + 6);
    uint32_t Xpsr = *(psp+7);
    uint32_t flags = NVIC_FAULT_STAT_R;
    uint32_t AdressF = NVIC_MM_ADDR_R;




    putsUart0("\nStack Dump Follows \n");
    putsUart0("\nProcess Stack Pointer:\n");
    numToString(stackP,Line,true,true);
    putsUart0(Line);
    putsUart0("\n Master Stack Pointer \n");
    numToString(stackMp,Line,true,true);
    putsUart0(Line);
    putsUart0("\nRegisters R0 - R3 \n");
    numToString(R0,Line,true,false);
    putsUart0(Line);
    putsUart0("\n");
    numToString(R1,Line,true,false);
    putsUart0(Line);
    putsUart0("\n");
    numToString(R2,Line,true,false);
    putsUart0(Line);
    putsUart0("\n");
    numToString(R3,Line,true,false);
    putsUart0(Line);
    putsUart0("\n");
    putsUart0("R12 :  ");
    numToString(R12,Line,true,false);
    putsUart0(Line);
    putsUart0("\n");
    putsUart0("Xpsr:  ");
    numToString(Xpsr,Line,true,false);
    putsUart0(Line);
    putsUart0("\n");
    putsUart0("LR\t");
    numToString(LR,Line,true,false);
    putsUart0(Line);
    putsUart0("\n");
    putsUart0("PC\t");
    numToString(PC,Line,true,false);
    putsUart0(Line);
    putsUart0("\n");
    putsUart0("Offending Instruction:    ");
    numToString(AdressF,Line,true,true);
    putsUart0(Line);
    putsUart0("\n");
    putsUart0("Flags follow : \t");
    numToString(flags,Line,true,false);
    putsUart0(Line);



    putsUart0("\n");

    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;

}

// REQUIRED: code this function
void hardFaultIsr(void)
{
        putsUart0(" \n Hard Fault has occurred \n");


        char Line[12];


        uint32_t stackMp = getMsp();
        uint32_t stackP = getPsp();
        uint32_t *psp = (uint32_t*)stackP;
        uint32_t R0 = *psp;
        uint32_t R1 = *(psp+1);
        uint32_t R2 = *(psp+2);
        uint32_t R3 = *(psp+3);

        uint32_t R12 = *(psp + 4);

        uint32_t LR = *(psp + 5);
        uint32_t PC = *(psp + 6);
        uint32_t Xpsr = *(psp+7);
        uint32_t flags = NVIC_FAULT_STAT_R;




        putsUart0("\nStack Dump Follows \n");
        putsUart0("\nProcess Stack Pointer:\n");
        numToString(stackP,Line,true,true);
        putsUart0(Line);
        putsUart0("\n Master Stack Pointer \n");
        numToString(stackMp,Line,true,true);
        putsUart0(Line);
        putsUart0("\nRegisters R0 - R3 \n");
        numToString(R0,Line,true,false);
        putsUart0(Line);
        putsUart0("\n");
        numToString(R1,Line,true,false);
        putsUart0(Line);
        putsUart0("\n");
        numToString(R2,Line,true,false);
        putsUart0(Line);
        putsUart0("\n");
        numToString(R3,Line,true,false);
        putsUart0(Line);
        putsUart0("\n");
        putsUart0("R12 :  ");
        numToString(R12,Line,true,false);
        putsUart0(Line);
        putsUart0("\n");
        putsUart0("Xpsr:  ");
        numToString(Xpsr,Line,true,false);
        putsUart0(Line);
        putsUart0("\n");
        putsUart0("LR\t");
        numToString(LR,Line,true,false);
        putsUart0(Line);
        putsUart0("\n");
        putsUart0("PC\t");
        numToString(PC,Line,true,false);
        putsUart0(Line);
        putsUart0("\n");
        putsUart0("Flags follow :    ");
        numToString(flags,Line,true,false);
        putsUart0(Line);
        putsUart0("\n");



        while(1);

}

// REQUIRED: code this function
void busFaultIsr(void)
{
    putsUart0("\n Bus Fault has occured PID: 69 \n");

    while(1);
}

// REQUIRED: code this function
void usageFaultIsr(void)
{
   putsUart0("\n Usage Fault has occurred PID: 420 \n");
   NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_DIV0;
   NVIC_CFG_CTRL_R &= ~NVIC_CFG_CTRL_DIV0;

   while(1);
}

