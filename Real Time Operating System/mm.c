// Memory manager functions
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

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

//    00000000  00000000  00000000  00001000  00000000  00000000  00000000  00000000
//     Reg 1     Reg 2     Reg 3     Reg 4     Reg 5     Reg 6     Reg 7     Reg 8
//    All Mem    Flash     Periph     4K        8K        4K        4K        8K

// 1 => Sub_Region in use ; 0 = > Sub_Region not used

extern char g_stack_heap;

PROCESS programs[14];
uint8_t progCount = 0;
uint64_t inUse;

// REQUIRED: add your malloc code here and update the SRD bits for the current thread
void * mallocFromHeap(uint32_t size_in_bytes)
{

    if(size_in_bytes <= 512)
    {
       // Check in regions 4 , 6, and 7

       // Extract Regions 4 , 6, and 7
        bool oneBlockFound512 = false;
        bool oneBlockFound1k = false;
        int8_t reg_avail = 3;

        uint8_t region = 4; // Region 4 is our starting position
        uint8_t subregions = (inUse >>32) & 0xFF; // Region 4
        int8_t i;
        uint8_t index;
        uint8_t mask;
        bool subreg_found = false;

        while((reg_avail > 0))
        {
            if(subregions < 0XFF)
            {
                //Find what Sub-Regions are free

                for(i = 7; i >= 0; i--)
                {
                    mask = 1 << i;
                    if(((subregions & mask) == 0) && (!subreg_found))
                    {
                        index = i;
                        subreg_found = true; // At least one 512 B region is free
                        oneBlockFound512 = true;
                    }
                }

                if(subreg_found)
                {
                    uint64_t shiftValue = 0x00000000;
                    void * temp = NULL;

                    programs[progCount].Size = 512;
                    programs[progCount].start_Bit = index;
                    programs[progCount].numOfBits = 1;
                    programs[progCount].SpecialReg = false;
                    // return addr
                    subregions |= (1 << index);
                    shiftValue = subregions;
                    if(region == 4)
                    {
                        inUse |= (shiftValue << 32);
                        temp = ((&g_stack_heap + (0x200 * (7 - index))));
                        programs[progCount].Address = temp;
                        programs[progCount].regionNum = region;
                        progCount++;
                        return temp;

                    }
                    else if(region == 6)
                    {
                        inUse |= (shiftValue << 16);
                        temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x200 * (7 -index))));
                        programs[progCount].Address = temp;
                        programs[progCount].regionNum = region;
                        progCount++;
                        return temp;

                    }
                    else if(region == 7)
                    {

                        inUse |= (shiftValue << 8);
                        temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x1000) + (0x200 * (7 -index))));
                        programs[progCount].Address = temp;
                        programs[progCount].regionNum = region;
                        progCount++;
                        return temp;

                    }
                }

            }
            else
            {
                reg_avail--;

                if(reg_avail == 2)
                {
                    region = 6;
                    subregions = (inUse >> 16) & 0xFF; // Region 6

                }

                if(reg_avail == 1)
                {
                    region = 7;
                    subregions = (inUse >> 8) & 0xFF; // Region 7;
                }
            }

        }

        if(!oneBlockFound512) // If we fail to find a 512 block, try to find a 1024 block
        {
            reg_avail = 2;
            region = 5;
            subregions = (inUse >> 24) & 0xFF;
            uint8_t mask;

            while(reg_avail > 0)
            {
                if(subregions < 0xFF)
                {
                    for(i = 7; i >= 0; i--)
                    {
                        mask = 1 <<i;
                        if(((subregions & mask) == 0) && (!subreg_found))
                        {
                            index = i;
                            subreg_found = true;
                            oneBlockFound1k = true;
                        }
                    }

                    if(subreg_found)
                    {
                        uint64_t shiftValue = 0x00000000;
                        void * temp = NULL;

                        programs[progCount].Size = 1024;
                        programs[progCount].start_Bit = index;
                        programs[progCount].numOfBits = 1;
                        programs[progCount].SpecialReg = false;
                        subregions |= (1 << index);
                        shiftValue = subregions;

                        if(region == 5)
                        {

                           inUse |= (shiftValue << 24);
                           temp = ((&g_stack_heap + (0x1000) + (0x400 * (7 -index))));
                           programs[progCount].Address = temp;
                           programs[progCount].regionNum = region;
                           progCount++;
                           return temp;

                        }
                        else if(region == 8)
                        {
                           inUse |= (shiftValue << 0);
                           temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x1000) + (0x1000) + (0x200 * (7 -index))));
                           programs[progCount].Address = temp;
                           programs[progCount].regionNum = region;
                           progCount++;
                           return temp;
                        }
                    }
                }
                else
                {
                     reg_avail--;

                     if(reg_avail == 1)
                     {
                         region = 8;
                         subregions = (inUse >> 0) & 0xFF;
                     }
                }
            }

        }

        if((!oneBlockFound512) && (!oneBlockFound1k)) // Could not find one 512 or 1k block
        {
            return NULL;
        }


    }
    else if((size_in_bytes > 512) && (size_in_bytes <= 1024))
    {
        // We want to save our 1024 sub-regions as much as possible
        //    to meet user memory demands

        // First find two continous 512 sub regions

        bool twoBlockFound_512 = false;
        bool oneBlockFound_1K = false;

        int8_t reg_avail = 2;
        int8_t i;
        int8_t selectreg = 5;
        uint8_t index;
        uint8_t Mask;
        bool subreg_found = false;
        bool switchReg = false;

        uint8_t subregions = (inUse >> 24) & 0xFF; // Start at region 4 again


        // we try to find one 1024 sub region in the 8k regions

        while((reg_avail > 0))
        {
           if(subregions < 0xFF)
           {
               for(i  = 7; i >= 0; i--)
               {
                   Mask = 1 << i;
                   if(((subregions & Mask) == 0) &&(!subreg_found))
                   {
                      index = i;
                      subreg_found = true;
                      oneBlockFound_1K = true;
                   }
               }

               if(subreg_found)
               {
                   //Return Address
                   uint64_t shiftValue = 0x00000000;
                   void * temp = NULL;

                   programs[progCount].Size = 1024;
                   programs[progCount].start_Bit = index;
                   programs[progCount].numOfBits = 1;
                   programs[progCount].SpecialReg = false;

                   subregions |= (1 << index);
                   shiftValue = subregions;
                   if(selectreg == 5)
                   {
                       inUse |= (shiftValue << 24);
                       temp = ((&g_stack_heap + (0x1000) + (0x400 * (7 -index))));
                       programs[progCount].Address = temp;
                       programs[progCount].regionNum = selectreg;
                       progCount++;
                       return temp;
                   }
                   else if(selectreg == 8)
                   {
                       inUse |= (shiftValue << 0);
                       temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x1000) + (0x1000) + (0x200 * (7 -index))));
                       programs[progCount].Address = temp;
                       programs[progCount].regionNum = selectreg;
                       progCount++;
                       return temp;
                   }
               }
           }
           else
           {
               reg_avail--;

               if(reg_avail == 1)
               {
                   subregions = (inUse >> 0) & 0xFF;
                   selectreg = 8;

               }

           }
        }

        if(!oneBlockFound_1K)
        {
            selectreg = 4;
            reg_avail = 2;
            subregions = (inUse >> 32) & 0xFF;
            bool first_found = false;


            while((reg_avail > 0))
            {
                if(subregions < 0xFF && (!switchReg) )
                {

                    for(i = 7; i >= 0; i--)
                    {
                        Mask = 1 << i;
                        if(((subregions & Mask) == 0) && (!subreg_found))
                        {
                            if(!first_found)
                            {
                                index = i;
                                first_found = true;
                            }
                            else if(((index - i) == 1))
                            {
                                subreg_found = true;
                                twoBlockFound_512 = true;
                            }
                            else
                            {
                                index = i;
                            }

                        }
                    }

                    if(subreg_found)
                    {
                        // return address
                        uint64_t shiftValue = 0x00000000;
                        void * temp =  NULL;

                        programs[progCount].Size = 1024;
                        programs[progCount].start_Bit = index;
                        programs[progCount].numOfBits = 2;
                        programs[progCount].SpecialReg = false;

                        subregions |= (1 << index) | (1 << (index - 1));
                        shiftValue = subregions;
                        if(selectreg == 4)
                        {
                            inUse |= (shiftValue << 32);
                            temp = ((&g_stack_heap + (0x200 * (7 -index))));
                            programs[progCount].Address = temp;
                            programs[progCount].regionNum = selectreg;
                            progCount++;
                            return temp;
                        }
                        else if(selectreg == 6)
                        {
                            inUse |= (shiftValue << 16);
                            temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x200 * (7 -index))));
                            programs[progCount].Address = temp;
                            programs[progCount].regionNum = selectreg;
                            progCount++;
                            return temp;
                        }
                        else if(selectreg == 7)
                        {
                            inUse |= (shiftValue << 8);
                            temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x1000) + (0x200 * (7 - index))));
                            programs[progCount].Address = temp;
                            programs[progCount].regionNum = selectreg;
                            progCount++;
                            return temp;
                        }
                    }
                    else
                    {
                        switchReg = true;
                    }
                }
                else
                {
                    reg_avail--;

                    if(reg_avail == 2)
                    {
                        selectreg = 6;
                        subregions = (inUse >> 16) & 0xFF; // Region 6

                    }

                    if(reg_avail == 1)
                    {
                        selectreg = 7;
                        subregions = (inUse >> 8) & 0xFF; // Region 7;
                    }

                    first_found = false;
                    switchReg = false;
                    first_found = false;

                }

            }

        }


        if((!oneBlockFound_1K) && (!twoBlockFound_512)) // If neither were found, we return NULL
        {
           return NULL;
        }


    }
    else if((size_in_bytes > 1024) && (size_in_bytes <= 1536))
    {
        // First we find if any of the 3 locations between 4K and 8k are available

        bool specialRegFound = false;
        bool threeregFound_512 = false;
        bool tworegFound_1K = false;
        bool subreg_found = false;
        bool first_check = false;
        uint8_t subreg1 = (inUse >> 32) & 0xFF; // 4k (Region 4)
        uint8_t subreg2 = (inUse >> 24) & 0xFF; // 8k (Region 5)
        uint8_t subregs;
        uint64_t shiftValue;
        int8_t select_reg = 4;
        int8_t avail_reg = 3;
        int8_t i;
        int8_t index;


        // if available subreg1 (4K) should have bit 0 available and subreg2 (8k) should
       // have bit 7 available
       // if not, we look through possible options

        while(avail_reg >0)
        {
            uint8_t Mask1 = (1 << 0);
            uint8_t Mask2 = (1 << 7);
            if(((subreg1 & Mask1) == 0) && ((subreg2 & Mask2) == 0))
            {
                subreg_found = true;
                specialRegFound = true;
            }

            if(subreg_found)
            {
                uint64_t shiftValue1 = 0x00000000;
                uint64_t shiftValue2 = 0x00000000;
                void * temp = NULL;

                subreg1 |= (1<<0);
                subreg2 |= (1 << 7);
                shiftValue1 = subreg1;
                shiftValue2 = subreg2;

                programs[progCount].Size = 1536;
                programs[progCount].start_Bit = 7;
                programs[progCount].numOfBits = 2;
                programs[progCount].SpecialReg = true;

                if(select_reg == 4)
                {

                    // regions 4 and 5
                    inUse |= (shiftValue1 << 32) | (shiftValue2 << 24);
                    temp = (&g_stack_heap + (7 * 0x200)); // We want the last subregion in the 4k section as the start addr
                    programs[progCount].Address = temp;
                    programs[progCount].regionNum = select_reg;
                    progCount++;
                    return temp;
                }
                else if(select_reg == 5)
                {
                    // regions 5 and 6
                    inUse |= (shiftValue1 << 24) | (shiftValue2 << 16);
                    temp = (&g_stack_heap + (0x1000) + (7*0x400)); // We want region 5 as the start addr
                    programs[progCount].Address = temp;
                    programs[progCount].regionNum = select_reg;
                    progCount++;
                    return temp;
                }
                else if(select_reg == 7)
                {
                    // regions 7 and 8
                    inUse |= (shiftValue1 << 8) | (shiftValue2 << 0);
                    temp = (&g_stack_heap + (0x1000) + (0x2000) + (0x1000) + (0x200 * 7 )); // we want region 7 as the start of addr
                    programs[progCount].Address = temp;
                    programs[progCount].regionNum = select_reg;
                    progCount++;
                    return temp;
                }
            }
            else
            {
                avail_reg--;

                if(avail_reg == 2)
                {
                    // if the intersection between region 4 and 5 doesn't work
                    // use the intersection between region 5 and 6
                    subreg1 = (inUse >> 24) & 0xFF;
                    subreg2 = (inUse >> 16) & 0xFF;
                    select_reg = 5;
                }

                if(avail_reg == 1)
                {
                    //if the intersection between region 5 and 6 doesn't work
                    //use the intersection between region 7 and 8
                    subreg1 = (inUse >> 8) & 0xFF;
                    subreg2 = (inUse >> 0) & 0xFF;
                    select_reg =7;
                }
            }
        }

        if(!specialRegFound)
        {
            //If the special regions are taken, find 3 consecutive 512 blocks

            select_reg = 4;
            subreg_found = false;
            avail_reg = 3; // 3 4k regions to use
            subregs = (inUse >>32) & 0xFF;
            int8_t check = 2;
            int8_t origin = 99;
            bool switchReg = false;
            uint8_t Mask;

            while(avail_reg > 0)
            {
                if(subregs < 0xFF && (!switchReg))
                {

                    for(i = 7; i >= 0; i--)
                    {
                        Mask = 1 << i;
                        if(((subregs & Mask) == 0)&&(!subreg_found))
                        {
                            if(!first_check)
                            {
                                index = i;
                                first_check = true;
                                origin = i;
                            }
                            else if(((index - i) == 1))
                            {
                                check--;
                                index = i;
                            }
                            else
                            {
                                index = i;
                                origin = i; // If we find a new zero, reset evreything
                                check = 2;
                            }

                            if(check == 0)
                            {
                                // adrr found
                                subreg_found = true;
                                threeregFound_512 = true;
                            }

                        }
                    }

                    if(subreg_found)
                    {
                        void* temp = NULL;
                        shiftValue = 0x00000000;
                        subregs |= (1<<origin) | (1 << (origin -1)) | (1 << (origin -2));
                        shiftValue = subregs;

                        programs[progCount].Size = 1536;
                        programs[progCount].start_Bit = origin;
                        programs[progCount].numOfBits = 3;
                        programs[progCount].SpecialReg = false;

                        // return address
                        if(select_reg == 4)
                        {
                            inUse |= (shiftValue << 32);
                            temp = ((&g_stack_heap + (0x200 * (7 -origin))));
                            programs[progCount].Address = temp;
                            programs[progCount].regionNum = select_reg;
                            progCount++;
                            return temp;
                        }
                        else if(select_reg == 6)
                        {
                            inUse |= (shiftValue << 16);
                            temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x200 * (7 -origin))));
                            programs[progCount].Address = temp;
                            programs[progCount].regionNum = select_reg;
                            progCount++;
                            return temp;
                        }
                        else if(select_reg == 7)
                        {
                            inUse |= (shiftValue << 8);
                            temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x1000) + (0x200 * (7 - origin))));
                            programs[progCount].Address = temp;
                            programs[progCount].regionNum = select_reg;
                            progCount++;
                            return temp;
                        }
                    }
                    else
                    {
                        switchReg = true;
                    }

                }
                else
                {
                    avail_reg--;

                    if(avail_reg == 2)
                    {
                        select_reg = 6;
                        subregs = (inUse >> 16) & 0xFF; // Region 6
                    }

                    if(avail_reg == 1)
                    {
                        select_reg = 7;
                        subregs = (inUse >> 8) & 0xFF; // Region 7
                    }

                    first_check = false;
                    switchReg = false;
                    subreg_found = false;
                }

            }

        }

        if(!threeregFound_512)
        {
            // If we have failed to find three consecutive 512 B subregions
            // find two 1k regions

            subregs = (inUse >> 24) & 0xFF;
            subreg_found = false;
            avail_reg = 2;
            select_reg = 5; // we start with the first 8K region
            first_check = false;
            bool switchReg = false;
            uint8_t Mask;
            while(avail_reg > 0)
            {
                if(subregs < 0xFF && (!switchReg))
                {
                    for(i = 7; i >= 0; i--)
                    {
                        Mask = 1 << i;
                        if(((subregs & Mask) == 0) && (!subreg_found))
                        {
                            if(!first_check)
                            {
                                index = i;
                                first_check = true;
                            }
                            else if((index - i) == 1)
                            {
                                subreg_found = true;
                                tworegFound_1K = true;
                            }
                            else
                            {
                                index = i;
                            }

                        }
                    }

                    if(subreg_found)
                    {
                         void * temp = NULL;
                         shiftValue = 0x00000000;
                         subregs |= (1 << index) | (1 << (index - 1));
                         shiftValue = subregs;

                        programs[progCount].Size = 1536;
                        programs[progCount].start_Bit = index;
                        programs[progCount].numOfBits = 2;
                        programs[progCount].SpecialReg = false;

                        // return addr
                        if(select_reg == 5)
                        {
                          inUse |= (shiftValue << 24);
                          temp = ((&g_stack_heap + (0x1000) + (0x400 * (7 -index))));
                          programs[progCount].Address = temp;
                          programs[progCount].regionNum = select_reg;
                          progCount++;
                          return temp;
                        }
                        else if(select_reg == 8)
                        {
                          inUse |= (shiftValue << 0);
                          temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x1000) + (0x1000) + (0x200 * (7 -index))));
                          programs[progCount].Address = temp;
                          programs[progCount].regionNum = select_reg;
                          progCount++;
                          return temp;
                        }
                    }
                    else
                    {
                        switchReg = true;
                    }
                }
                else
                {
                    avail_reg--;

                    if(avail_reg == 1)
                    {
                        subreg_found = (inUse >> 0) & 0xFF;
                        select_reg = 8;
                    }

                    first_check = false;
                    switchReg = false;
                    subreg_found = false;

                }
            }

        }

        // If evreything fails, return NULL
        if((!specialRegFound) && (!threeregFound_512) && (!tworegFound_1K))
        {
            return NULL;
        }

    }
    else if((size_in_bytes > 1536))
    {
        //We will divide the size needed into chunks of 1024

        uint16_t regNeeded = (size_in_bytes/1024);
        uint16_t regNeeded512 = (regNeeded * 2);
        uint8_t Mask;


        if(regNeeded > 8)
        {
            return NULL; //Insufficent consecutive memory
        }
        else if(size_in_bytes == 5000)
        {
            Mask = 127; // 01111111 in binary
            uint64_t subMask = Mask;
            inUse |= (subMask << 16);
            Mask = 224;// 11100000 in binary
            subMask = Mask;
            inUse |= (subMask << 8);

            programs[progCount].Size = (1024 * 5);
            programs[progCount].start_Bit = 6;
            programs[progCount].numOfBits = 7;
            programs[progCount].regionNum = 6;
            programs[progCount].SpecialReg = false;
            programs[progCount].isMultReg = true;
            programs[progCount].start_adBit = 7;
            programs[progCount].adnumofBits = 3;
            programs[progCount].adregionNum = 7;

            programs[progCount].Address = ((&g_stack_heap + (0x1000) + (0x2000) + (0x200 * 1)));
            progCount++;
            return ((&g_stack_heap + (0x1000) + (0x2000) + (0x200 * 1)));
        }
        else
        {
            uint8_t subreg = (inUse >> 24) & 0xFF;

            uint8_t index;
            uint8_t origin;
            bool regionFound1k = false;
            bool regionFound512 = false;
            int8_t select_reg = 5;
            int8_t avail_reg = 2;

            bool subregFound = false;
            bool firstCheck = false;
            bool switchReg = false;
            int8_t i;
            uint32_t regCopy = regNeeded;
            // Extract the number of subregions available
            while(avail_reg > 0)
            {
                if(subreg < 0xFF && (!switchReg))
                {
                    for(i = 7; i >= 0; i--)
                    {
                        Mask =  1 << i;
                        if(((subreg & Mask) == 0) && (!subregFound))
                        {
                            if(!firstCheck)
                            {
                                index = i;
                                origin = i;
                                firstCheck = true;
                                regCopy--;
                            }
                            else if((index - i) == 1)
                            {
                                regCopy--;
                                index = i;
                            }
                            else
                            {
                                regCopy = regNeeded;
                                index = i;
                                origin = i;
                            }
                        }

                        if(regCopy == 0)
                        {
                           subregFound = true;
                           regionFound1k = true;
                        }
                    }

                    if(subregFound)
                    {

                        int16_t j;
                        uint64_t shiftValue;
                        int16_t Value;
                        void * temp = NULL;

                        programs[progCount].Size = (1024 * regNeeded);
                        programs[progCount].start_Bit = origin;
                        programs[progCount].numOfBits = regNeeded;
                        programs[progCount].SpecialReg = false;

                        Value = (origin - (regNeeded -1));

                        for(j = origin; j >= Value; j--)
                        {
                            subreg |= (1 << j);
                        }
                        shiftValue = subreg;
                        //Memory Found
                        if(select_reg == 5)
                        {
                          inUse |= (shiftValue << 24);
                          temp = ((&g_stack_heap + (0x1000) + (0x400 * (7 -origin))));
                          programs[progCount].Address = temp;
                          programs[progCount].regionNum = select_reg;
                          progCount++;
                          return temp;
                        }
                        else if(select_reg == 8)
                        {
                          inUse |= (shiftValue << 0);
                          temp  = ((&g_stack_heap + (0x1000) + (0x2000) + (0x1000) + (0x1000) + (0x400 * (7 -origin))));
                          programs[progCount].Address = temp;
                          programs[progCount].regionNum = select_reg;
                          progCount++;
                          return temp;
                        }

                    }
                    else
                    {
                        switchReg = true;
                    }
                }
                else
                {
                   avail_reg--;

                   if(avail_reg == 1)
                   {
                       subreg = (inUse >> 0) & 0xFF; // Region 8 used instead
                       select_reg = 8;
                       switchReg = false;
                       firstCheck = false;
                       subregFound = false;
                       regCopy = regNeeded;
                   }
                }
            } // If we were unable to find sufficent memory in the 8k region
                // we could try the 4k regions
                // however, we will need 2 blocks to make 1024
            // thus (blocks needed * 2) < 8 ( in theory, the most that can fit is 4K)

            if(!regionFound1k && (regNeeded512 <= 8))
            {
                select_reg = 4; // For the 512 blocks, we will need region 4
                avail_reg = 3;
                Mask = 0;
                subreg = (inUse >> 32) & 0xFF;
                subregFound = false;
                firstCheck = false;
                switchReg = false;
                regCopy = regNeeded512;

                while(avail_reg > 0)
                {
                   if(subreg < 0xFF && (!switchReg))
                   {
                       for(i = 7; i >= 0; i--)
                       {
                           Mask =  1 << i;
                           if(((subreg & Mask) == 0) && (!subregFound))
                           {
                               if(!firstCheck)
                               {
                                   index = i;
                                   origin = i;
                                   firstCheck = true;
                                   regCopy--;
                               }
                               else if((index - i) == 1)
                               {
                                   regCopy--;
                                   index = i;
                               }
                               else
                               {
                                   regCopy = regNeeded512;
                                   index = i;
                                   origin = i;
                               }
                           }

                           if(regCopy == 0)
                           {
                              subregFound = true;
                              regionFound512 = true;
                           }
                       }

                       if(subregFound)
                       {

                           int16_t j;
                           uint64_t shiftValue;
                           int16_t Value;
                           void * temp = NULL;

                           programs[progCount].Size = (1024 * regNeeded);
                           programs[progCount].start_Bit = origin;
                           programs[progCount].numOfBits = regNeeded;
                           programs[progCount].SpecialReg = false;

                           Value = (origin - (regNeeded -1));

                           for(j = origin; j >= Value; j--)
                           {
                               subreg |= (1 << j);
                           }
                           shiftValue = subreg;
                           //Memory Found
                           if(select_reg == 4)
                           {
                               inUse |= (shiftValue << 32);
                               temp = ((&g_stack_heap + (0x200 * (7 -origin))));
                               programs[progCount].Address = temp;
                               programs[progCount].regionNum = select_reg;
                               progCount++;
                               return temp;
                           }
                           else if(select_reg == 6)
                           {
                               inUse |= (shiftValue << 16);
                               temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x200 * (7 -origin))));
                               programs[progCount].Address = temp;
                               programs[progCount].regionNum = select_reg;
                               progCount++;
                               return temp;
                           }
                           else if(select_reg == 7)
                           {
                               inUse |= (shiftValue << 8);
                               temp = ((&g_stack_heap + (0x1000) + (0x2000) + (0x1000) + (0x200 * (7 - origin))));
                               programs[progCount].Address = temp;
                               programs[progCount].regionNum = select_reg;
                               progCount++;
                               return temp;
                           }

                       }
                       else
                       {
                           switchReg = true;
                       }
                   }
                   else
                   {
                      avail_reg--;

                      if(avail_reg == 2)
                      {
                          select_reg = 6;
                          subreg = (inUse >> 16) & 0xFF; // Region 6
                      }

                      if(avail_reg == 1)
                      {
                          select_reg = 7;
                          subreg = (inUse >> 8) & 0xFF; // Region 7
                      }

                       firstCheck = false;
                       subregFound = false;
                       switchReg = false;
                       regCopy = regNeeded512;
                    }
                  }
               }
               else if((!regionFound512) && (!regionFound1k))
               {

                  return NULL;

               }
         }

    }

    return NULL;
}

// REQUIRED: add your free code here and update the SRD bits for the current thread
void freeToHeap(void *pMemory)
{
        uint8_t i;
        int8_t j;
        int8_t stopbit;
        uint8_t index;
        uint8_t Mask = 0;;
        uint8_t Bitp;
        uint8_t bitSize;
        uint8_t Region;
        uint64_t shiftMask;
        bool Check;

        for(i = 0; i < progCount; i++)
        {
            if(programs[i].Address == pMemory)
            {
                index = i;
            }

        }

        Bitp = programs[index].start_Bit;
        bitSize = programs[index].numOfBits;
        Region = programs[index].regionNum;
        Check = programs[index].SpecialReg;

        if(Check == true)
        {
            Mask = (1 << 0);
            uint8_t Mask2 = (1 << 7);
            //Depending on the starting region, the bit shift will differ
            if(Region == 4)  // The intersection between regions 4 and 5
            {
                shiftMask = Mask;
                inUse &= ~(shiftMask << 32);

                shiftMask = Mask2;
                inUse &= ~(shiftMask << 24);

            }
            else if(Region == 5)
            {
                shiftMask = Mask;
                inUse &= ~(shiftMask << 24);

                shiftMask = Mask2;
                inUse &= ~(shiftMask << 16);

            }
            else if(Region == 7)
            {
                shiftMask = Mask;
                inUse &= ~(shiftMask << 8);

                shiftMask = Mask2;
                inUse &= ~(shiftMask << 0);
            }


        }
        else if(programs[index].isMultReg)
        {
            Mask = 127;
            shiftMask = Mask;
            inUse &= ~(shiftMask << 16);
            Mask = 224;
            shiftMask = Mask;
            inUse &= ~(shiftMask << 8);


            programs[progCount].isMultReg = false;
            programs[progCount].start_adBit = 0;
            programs[progCount].adnumofBits = 0;
            programs[progCount].adregionNum = 0;



        }
        else
        {


            stopbit = (Bitp - (bitSize -1));

            for(j = Bitp; j >= stopbit ; j--)
            {
                Mask |= (1 << j);
            }
            shiftMask = Mask;

            switch(Region){

             case 4:
                 inUse &= ~(shiftMask << 32);
                 break;

             case 5:
                 inUse &= ~(shiftMask << 24);
                 break;

             case 6:
                 inUse &= ~(shiftMask << 16);
                 break;

             case 7:
                 inUse &= ~(shiftMask << 8);
                 break;

             case 8:
                 inUse &= ~(shiftMask << 0);
                 break;

             default:
                 inUse = ~inUse;


            }

        }

        programs[index].start_Bit = 0;
        programs[index].numOfBits = 0;
        programs[index].regionNum = 0;
        programs[index].SpecialReg = false;
        programs[index].Address = NULL;
        programs[index].Size = 0;
        programs[index].isMultReg = false;


        for(i = index; i < progCount; i++)
        {
           programs[i].start_Bit = programs[i+1].start_Bit;
           programs[i].numOfBits = programs[i+1].numOfBits;
           programs[i].regionNum = programs[i+1].regionNum;
           programs[i].SpecialReg = programs[i+1].SpecialReg;
           programs[i].Address = programs[i+1].Address;
           programs[i].Size = programs[i+1].Size;
           programs[i].isMultReg = programs[i+1].isMultReg;

           if((programs[i].Size != 5120) && (programs[i].isMultReg == true))
           {
               programs[i].isMultReg = false;
           }
        }

        progCount--;


}

// REQUIRED: include your solution from the mini project
void allowFlashAccess(void)
{
    NVIC_MPU_CTRL_R &=  ~(NVIC_MPU_CTRL_ENABLE | NVIC_MPU_CTRL_PRIVDEFEN);

       // Size: 262,144 B
       // N = 18
       // S R C bits: 010 -> 0x2
       // Size = 17 -> 0x11
       // Start Addr = 0x0000.0000
       // Acesss: 011 -> 0x3 (All allowed)  //110 -> 0x6 (for testing) Read only for both


       NVIC_MPU_NUMBER_R = 0; // Choose region 0
       NVIC_MPU_BASE_R = ((0x00000000));
                          // XN bit     //AP bits      //SCB bits    //Size        //Enable
       NVIC_MPU_ATTR_R |= (0x0 << 27) | (0x3 << 24) | (0x2 << 16)| (0x11 << 1) | (0x1 << 0);
       NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_MEM;
}

void allowPeripheralAccess(void)
{
    NVIC_MPU_CTRL_R &=  ~(NVIC_MPU_CTRL_ENABLE | NVIC_MPU_CTRL_PRIVDEFEN);
        // Size: 67, 108,864
        // N = 26
        // S R C bits: 101 -> 0x5
        // Size = 25 -> 0x19
        // Start Addr: 0x4000.0000
        // Acesss: 011 -> 0x3 (All allowed)  //110 -> 0x6 (for testing) Read only for both
        NVIC_MPU_NUMBER_R = 1; /// Region 1
        NVIC_MPU_BASE_R = ((0x40000000));
                            // XN bit     //AP bits      //SCB bits    //Size        //Enable
        NVIC_MPU_ATTR_R |= (0x1 << 27) | (0x3 << 24) |  (0x5 << 16) | (0x19 << 1) | (0x1 << 0);
        NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_MEM;
}

void setupSramAccess(void)
{
    NVIC_MPU_CTRL_R &=  ~(NVIC_MPU_CTRL_ENABLE| NVIC_MPU_CTRL_PRIVDEFEN);
        NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEM;


        // Set up MPU regions in SRAM

        // Region 2: OS
        // Size : 4k
        // N = 12
        // S R C bits: 110 -> 0x6
        // Size : 11 -> 0xB
        // Start Addr: 0x2000.0000
        // Acesss: 001 -> 0x1 (Only privg can write/read)
        NVIC_MPU_NUMBER_R = 2;
        NVIC_MPU_BASE_R = (0x20000000);
                                        // Privg Access  // S R C bits  // Size     // Disable subregions
        NVIC_MPU_ATTR_R |= (0x1 << 27) | (0x3 << 24) | (0x6 << 16) | (0xB << 1) | (0xFF << 8);
                            //Enable
        NVIC_MPU_ATTR_R |= (0x1 << 0);

        // Region 3: 4K
        // Size : 4K
        // N = 12
        // S R C bits: 110 -> 0x6
        // Size: 11 -> 0xB
        // Start Addr: 0x2000.1000
        // Acesss: 001 -> 0x1
        NVIC_MPU_NUMBER_R = 3;
        NVIC_MPU_BASE_R = (0x20001000);
                        // Privg Access  // S R C bits  // Size     // Disable subregions
        NVIC_MPU_ATTR_R |= (0x1 << 27) | (0x3 << 24) | (0x6 << 16) | (0xB << 1)  | (0xFF << 8);
        NVIC_MPU_ATTR_R |= (0x1 << 0);


        // Region 4: 8K
        // Size: 8K
        // N = 13
        // S R C bits: 110 -> 0x6
        // Size: 12 -> 0xC
        // Start Addr: 0x2000.2000
        // Acess: 001 -> 0x1
        NVIC_MPU_NUMBER_R = 4;
        NVIC_MPU_BASE_R = (0x20002000);
                        // Privg Access  // S R C bits  // Size     // Disable subregions
        NVIC_MPU_ATTR_R |= (0x1 << 27) | (0x3 << 24) | (0x6 << 16) | (0xC << 1) | (0xFF << 8);
        NVIC_MPU_ATTR_R |= (0x1 << 0);


        // Region 5: 4K
        // Size : 4K
        // N = 12
        // S R C bits: 110 -> 0x6
        // Size: 11 -> 0xB
        // Start Addr: 0x2000.4000
        // Acesss: 001 -> 0x1
        NVIC_MPU_NUMBER_R = 5;
        NVIC_MPU_BASE_R = (0x20004000);
                        // Privg Access  // S R C bits  // Size    //Disable subregions
        NVIC_MPU_ATTR_R |= (0x1 << 27) | (0x3 << 24) | (0x6 << 16) | (0xB << 1) | (0xFF << 8);
        NVIC_MPU_ATTR_R |= (0x1 << 0);


        // Region 6: 4K
        // Size : 4K
        // N = 12
        // S R C bits: 110 -> 0x6
        // Size: 11 -> 0xB
        // Start Addr: 0x2000.4000
        // Acesss: 001 -> 0x1
        NVIC_MPU_NUMBER_R = 6;
        NVIC_MPU_BASE_R = (0x20005000);
                        // Privg Access  // S R C bits  // Size      //Disable subregions
        NVIC_MPU_ATTR_R |= (0x1 << 27) | (0x3 << 24) | (0x6 << 16) | (0xB << 1) | (0xFF << 8);
        NVIC_MPU_ATTR_R |= (0x1 << 0);


        // Region 7: 8K
        // Size : 8K
        // N = 13
        // S R C bits: 110 -> 0x6
        // Size: 12 -> 0xC
        // Start Addr: 0x2000.6000
        // Acesss: 001 -> 0x1
        NVIC_MPU_NUMBER_R = 7;
        NVIC_MPU_BASE_R = (0x20006000);
                         // Privg Access // S R C bits  // Size     //Disable subregions
        NVIC_MPU_ATTR_R |= (0x1 << 27) | (0x3 << 24) | (0x6 << 16) | (0xC << 1) | (0xFF << 8);
        NVIC_MPU_ATTR_R |= (0x1 << 0);


        NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_MEM;
}

uint64_t createNoSramAccessMask(void)
{
    return 0xFFFFFFFFFFFFFFFF;
}

void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes)
{
    //    00000000  00000000  00000000 00000000     00000000   00000000  00000000  00000000  00000000
       //   Background   Reg 0     Reg 1    Reg 2        Reg 3     Reg 4     Reg 5     Reg 6    Reg 7
       //    All Mem    Flash     Periph     OS          4K        8K        4K        4K        8K
                       // Reg 2 is the os, that reminas only RWX for Privigled unless testing/requested

       uint32_t* baseSram = (uint32_t *)0x20000000;
       if((baseAdd == baseSram) && (size_in_bytes == 32768))
       {
           uint64_t AcessMask = 0xFFFF000000000000;
           *srdBitMask &= AcessMask;

       }
       else if((baseAdd == baseSram) && (size_in_bytes == 4096 ))
       {
           uint64_t AcessMask = 0xFFFF00FFFFFFFFFF;
           *srdBitMask &= AcessMask;

       }
       else
       {
           // Only certain regions of SRAM should be enabled, this should work in conjuction with
           // Malloc's inUse variable since it directs to which subregion should be enabled

           //Given the pointer, finds it's corresponding struct

           uint8_t i;
           uint8_t index = 45;
           void * temp = NULL;

           temp = (void *)baseAdd;

           for(i = 0; i < progCount; i++)
           {
               if(programs[i].Address == temp)
               {
                   index = i;
               }
           }

           if(index == 45)
           {
               return;  // That block has been freed
           }
           else if(programs[index].SpecialReg)
           {
               //If it's  a special region, set specific bits in number
               uint8_t locReg = programs[index].regionNum;

               uint8_t Mask1 = (1 << 0);
               uint8_t Mask2 = (1 << 7);
               uint64_t shiftMask;

               if(locReg == 4)
               {
                   shiftMask = Mask1;
                   *srdBitMask &= ~(shiftMask << 32);

                   shiftMask = Mask2;
                   *srdBitMask &= ~(shiftMask << 24);
               }
               else if(locReg == 5)
               {
                   shiftMask = Mask1;
                   *srdBitMask &= ~(shiftMask << 24);

                   shiftMask = Mask2;
                   *srdBitMask &= ~(shiftMask << 16);
               }
               else if(locReg == 7)
               {
                   shiftMask = Mask1;
                   *srdBitMask &= ~(shiftMask << 8);

                   shiftMask = Mask2;
                   *srdBitMask &= ~(shiftMask << 0);
               }

               //*srdBitMask &= 0xFFFF00FFFFFFFFFF;

           }
           else if(programs[index].isMultReg)
           {
               uint8_t regionMask = 127;
               uint64_t shiftMask;

               shiftMask = regionMask;

               *srdBitMask &= ~(shiftMask << 16);

               regionMask = 224;
               shiftMask = regionMask;
               *srdBitMask &= ~(shiftMask << 8);

           }
           else
           {
               uint8_t location = programs[index].regionNum;
               int8_t j;
               uint8_t firstBit = programs[index].start_Bit;
               uint8_t numBits = programs[index].numOfBits;

               uint8_t lastbit = firstBit -(numBits -1);
               uint8_t regionMask = 0;

               for(j = firstBit; j >= lastbit; j--)
               {
                   regionMask |= (1 << j);
               }

               uint64_t shiftMask = regionMask;


               switch(location){

                case 4:
                    *srdBitMask &= ~(shiftMask << 32);
                    break;

                case 5:
                    *srdBitMask &= ~(shiftMask << 24);
                    break;

                case 6:
                    *srdBitMask &= ~(shiftMask << 16);
                    break;

                case 7:
                    *srdBitMask &= (~shiftMask << 8);
                    break;

                case 8:
                    *srdBitMask &= ~(shiftMask << 0);
                    break;

                default:
                    *srdBitMask = ~(*srdBitMask);


               }

              // *srdBitMask &= 0xFFFF00FFFFFFFFFF; // Enable OS subregions for testing



           }


       }




}

uint8_t flipNum(uint8_t num)
{
    uint8_t newNum = 0;
    uint8_t i = 0;

    for(i = 0; i < 8; i++)
    {
        newNum <<= 1;
        newNum |= (num & 1);
        num >>= 1;
    }

    return newNum;

}

void applySramAccessMask(uint64_t srdBitMask)
{

    //    00000000  00000000  00000000 00000000     00000000   00000000  00000000  00000000  00000000
      //   Background   Reg 0     Reg 1    Reg 2        Reg 3     Reg 4     Reg 5     Reg 6    Reg 7
      //    All Mem    Flash     Periph     OS          4K        8K        4K        4K        8K
                      // Reg 2 is the os, that reminas only RWX for Privigled unless testing/requested

    // Regions 1 - 3 can be ignored, we need to extract regions 4 - 8




    uint8_t subreg = (srdBitMask >> 40) & 0xFF; // Start at Region 2
    uint8_t flipsubreg;
    int8_t regCount = 5;
    uint32_t address = 0x20000000; // Start at OS
    uint32_t index = 2;



    while(regCount >= 0)
    {
        // 1 means allowed usage (since in Malloc a 1 means an allocated region)
        // 0 means that regions is not allowed usage (Malloc a 0 means regions is available for usage)
        // In the register, a 1 means a subregion is disabled, thus we must negate our mask
        // Flip the mask since the first bit in SRD indicates the first subregion

        flipsubreg = flipNum(subreg);
        uint16_t shiftMask = ((~flipsubreg) <<  8);




            NVIC_MPU_NUMBER_R = index;
            NVIC_MPU_ATTR_R &= ~(1 << 0);
            NVIC_MPU_BASE_R = address;
            NVIC_MPU_ATTR_R &= ~shiftMask;
            NVIC_MPU_ATTR_R |= (1 << 0);




        index++;

        switch(index)
        {
          case 3: // Reg 3
            subreg = (srdBitMask >> 32) & 0xFF;
            address = 0x20001000;
            regCount--;
          break;

          case 4: // Reg 4
            subreg = (srdBitMask >> 24) & 0xFF;
            address = 0x20002000;
            regCount--;
          break;

          case 5: //Reg 5
            subreg = (srdBitMask >> 16) & 0xFF;
            address = 0x20004000;
            regCount--;
          break;

          case 6: //Reg 6
            subreg = (srdBitMask >> 8) & 0xFF;
            address = 0x20005000;
            regCount--;
          break;

          case 7: // Reg 7
            subreg = (srdBitMask >> 0) & 0xFF;
            address = 0x20006000;
            regCount--;
          break;

          default:
             regCount--;
          break;


        }

    }





}


