// Memory manager functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef MM_H_
#define MM_H_

#define NUM_SRAM_REGIONS 4

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

typedef struct _PROCESS
{
    void * Address;
    uint16_t Size;
    uint8_t start_Bit;
    uint8_t regionNum;
    uint8_t numOfBits;
    uint8_t start_adBit;
    uint8_t adregionNum;
    uint8_t adnumofBits;
    bool SpecialReg;
    bool isMultReg;
}PROCESS;

void * mallocFromHeap(uint32_t size_in_bytes);
void freeToHeap(void *pMemory);

void allowFlashAccess(void);
void allowPeripheralAccess(void);
void setupSramAccess(void);
uint64_t createNoSramAccessMask(void);
void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes);
void applySramAccessMask(uint64_t srdBitMask);

#endif
