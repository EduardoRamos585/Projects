// Tasks
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
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "clock.h"
#include "kernel.h"
#include "tasks.h"
#include "nvic.h"

// Hardware Configuration
// Blue: PF2
// Red: PA2
// Orange:PA3
// Yellow: PA4
// Green: PE0
// PB's on pins
// PB0:     PC4
// PB1:     PC5
// PB2:     PC6
// PB3:     PC7
// PB4:     PD6
// PB5:     PD7

#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTA,2 // off-board red LED
#define ORANGE_LED PORTA,3 // off-board orange LED
#define YELLOW_LED PORTA,4 // off-board yellow LED
#define GREEN_LED  PORTE,0 // off-board green LED

#define PB0 PORTC,4
#define PB1 PORTC,5
#define PB2 PORTC,6
#define PB3 PORTC,7
#define PB4 PORTD,6
#define PB5 PORTD,7

#define PC4_MASK 16
#define PC5_MASK 32
#define PC6_MASK 64
#define PC7_MASK 128
#define PD6_MASK 64
#define PD7_MASK 128

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    // Setup LEDs and pushbuttons
    initSystemClockTo40Mhz();


    enablePort(PORTF);
    enablePort(PORTE);
    enablePort(PORTD);
    enablePort(PORTC);
    enablePort(PORTB);
    enablePort(PORTA);
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R0;


    _delay_cycles(3);


    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;
    WTIMER0_CFG_R = TIMER_CFG_32_BIT_TIMER;
    WTIMER0_TAMR_R = TIMER_TAMR_TAMR_1_SHOT;
    WTIMER0_TAILR_R = 40000000;

    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(ORANGE_LED);
    selectPinPushPullOutput(YELLOW_LED);
    selectPinPushPullOutput(BLUE_LED);

    setPinCommitControl(PORTD, 7);
    GPIO_PORTD_CR_R = 0x80;


    selectPinDigitalInput(PB0);
    selectPinDigitalInput(PB1);
    selectPinDigitalInput(PB2);
    selectPinDigitalInput(PB3);
    selectPinDigitalInput(PB4);
    selectPinDigitalInput(PB5);





    enablePinPullup(PB0);
    enablePinPullup(PB1);
    enablePinPullup(PB2);
    enablePinPullup(PB3);
    enablePinPullup(PB4);
    enablePinPullup(PB5);








    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_USAGE | NVIC_SYS_HND_CTRL_MEM | NVIC_SYS_HND_CTRL_BUS;

    //Set up the system tick isr




    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);
}

// REQUIRED: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs(void)
{
    //  0    0     0     0     0     0
 //    2^5  2^4   2^3   2^2   2^1   2^0
//    PB0   PB1  PB2    PB3   PB4   PB5
//    PC4   PC5  PC6    PC7   PD6   PD7

    uint8_t PIN = 0;
    if(!getPinValue(PB5)) PIN |= 1;
    if(!getPinValue(PB4)) PIN |= 2;
    if(!getPinValue(PB3)) PIN |= 4;
    if(!getPinValue(PB2)) PIN |= 8;
    if(!getPinValue(PB1)) PIN |= 16;
    if(!getPinValue(PB0)) PIN |= 32;


    return PIN;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while(true)
    {
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        yield();
    }
}


void idle2(void)
{
    while(true)
    {
        setPinValue(YELLOW_LED, 1);
        waitMicrosecond(1000);
        setPinValue(YELLOW_LED, 0);
        yield();
    }
}

void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    uint8_t *mem;
    mem = mallocCall(5000 * sizeof(uint8_t));
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
            mem[i] = i % 256;
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            stopThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}
