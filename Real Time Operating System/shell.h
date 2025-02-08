// Shell functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef SHELL_H_
#define SHELL_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
#include <stdbool.h>
#include <stdio.h>
#include "uart0.h"
#include "kernel.h"

#define MAX_CHARS 80
#define MAX_FIELDS 5

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];

}USER_DATA;

void stringCopy(char * L1,  const char * L2);

void numToString(uint32_t value, char* buffer, bool printHex, bool alreadyHex);
void stringCat(char * sent1, char * sent2, char * result);
uint8_t strLen(const char * sent);
bool stringCompare(char * L1, char* L2);


void shell(void);

#endif
