// Eduardo Ramos 11/2/2023 Lab 7

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "clock.h"
//#include "driverlib/eeprom.h"
#include "eeprom.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "wait.h"


#define SENSOR     (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 2*4))) //SENSOR PB2
#define SPEAKER   (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 5*4)))
#define FOOD     (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 7*4))) // PB7   // food, water, speaker, sensor
#define WATER     (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 5*4))) // PC5

#define INPUT (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 3*4)))   // Input for discharging capacitor

#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

#define GREEN_LED_MASK 8


#define MAX_CHARS 80    // Masks for UART config
#define MAX_FIELDS 5

#define MOTION_MASK 4
#define SPEAKER_GPIO_MASK 32  // Masks for configuring hardware
#define WATER_MASK  32
#define FOOD_MASK 128

#define INPUT_MASK 8
#define C0Plus_MASK 64    // Masks for configuring bowl capacitor
#define C0Minus_MASK 128

//#define DEBUG


uint32_t time = 0;
char str[100];
int event_alarm = 0;
int pre_flag = 0;
int fill_mode_t = 5;
uint32_t expected_lv = 0;
int32_t current_level = 0;
int alarm_status = 5;
int32_t lower_level = 100;
int32_t current_tick = 0;
int32_t previous_tick = 0;



typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];

}USER_DATA;



//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{

    // Initialize system clock to 40 MHz
       initUart0();
       setUart0BaudRate(115200,40e6);
      initSystemClockTo40Mhz();
      initEeprom();


      // Enable clocks
     // SYSCTL_RCGCEEPROM_R |= SYSCTL_RCGCEEPROM_R0;
      SYSCTL_RCGCHIB_R = SYSCTL_RCGCHIB_R0;  // Enable hibernation module
      SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1|SYSCTL_RCGCTIMER_R2|SYSCTL_RCGCTIMER_R3|SYSCTL_RCGCTIMER_R4; // Enable 32/16 bit timer
      SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1|SYSCTL_RCGCWTIMER_R0; // Enable 64/32 bit timer
      SYSCTL_RCGCACMP_R |= SYSCTL_RCGCACMP_R0; // Enable comparator
      SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5 |SYSCTL_RCGCGPIO_R0 | SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R2;
      _delay_cycles(3);



      //Registers to enable the RTC



      while((~HIB_CTL_R & HIB_CTL_WRC) );
          HIB_CTL_R = HIB_CTL_CLK32EN; //Enable osc


       while((~HIB_CTL_R & HIB_CTL_WRC) );
           HIB_IM_R = HIB_IM_RTCALT0;

       while((~HIB_CTL_R & HIB_CTL_WRC) );
        NVIC_EN1_R = 1 <<(INT_HIBERNATE-16-32);

        while((~HIB_CTL_R & HIB_CTL_WRC) );
         HIB_CTL_R |= HIB_CTL_RTCEN; //Enable osc input and Start the RTC clock




      GPIO_PORTF_DIR_R |= GREEN_LED_MASK; // Test for wide timer
      GPIO_PORTF_DEN_R |= GREEN_LED_MASK;

      GPIO_PORTB_DIR_R |= INPUT_MASK; // GPIO pin as output
      GPIO_PORTB_DEN_R |= INPUT_MASK; // Digital signal


      GPIO_PORTC_DIR_R &= ~C0Minus_MASK ;
      GPIO_PORTC_DEN_R &= ~C0Minus_MASK;


      GPIO_PORTC_AFSEL_R &= ~C0Minus_MASK;
      GPIO_PORTC_AMSEL_R |= C0Minus_MASK;

      COMP_ACREFCTL_R |= COMP_ACREFCTL_EN | COMP_ACREFCTL_VREF_M;
      COMP_ACREFCTL_R &= ~COMP_ACREFCTL_RNG;
      COMP_ACCTL0_R |= COMP_ACCTL0_ASRCP_REF | COMP_ACCTL0_CINV |COMP_ACCTL0_ISEN_RISE;


      WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
      WTIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;              // configure for periodic mode
      WTIMER1_TAILR_R = 400000000;
      WTIMER1_CTL_R |= TIMER_CTL_TAEN; //Turn on interrupts
      WTIMER1_IMR_R = TIMER_IMR_TATOIM;  // Turn on timer
      NVIC_EN3_R = 1 << (INT_WTIMER1A-16-96);

      TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
      TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;// turn-off timer before reconfiguring           // configure as 32-bit timer (A+B)
      TIMER1_TAMR_R |= TIMER_TAMR_TAMR_PERIOD|TIMER_TAMR_TACDIR ;



      // Configure LED and pushbutton pins
      GPIO_PORTB_DIR_R &= ~MOTION_MASK;

      GPIO_PORTB_DEN_R |= MOTION_MASK;

      GPIO_PORTA_DIR_R |= SPEAKER_GPIO_MASK;
      GPIO_PORTA_DEN_R |= SPEAKER_GPIO_MASK;

     // GPIO_PORTB_DIR_R |= FOOD_MASK;   //PB7   M0PWM1
      GPIO_PORTB_DEN_R |= FOOD_MASK;  // PB7
      GPIO_PORTB_AFSEL_R |= FOOD_MASK;
      GPIO_PORTB_PCTL_R &= ~(GPIO_PCTL_PB7_M);
      GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB7_M0PWM1;



      //GPIO_PORTC_DIR_R |= WATER_MASK; //PC5   M0PWM7
      GPIO_PORTC_DEN_R |= WATER_MASK; //PC5
      GPIO_PORTC_AFSEL_R |= WATER_MASK;
      GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC5_M);
      GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC5_M0PWM7;


      SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;
      SYSCTL_SRPWM_R = 0;
      PWM0_0_CTL_R = 0;
      PWM0_3_CTL_R = 0;

      PWM0_0_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO;
      PWM0_3_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO;


      PWM0_0_LOAD_R = 1024;
      PWM0_3_LOAD_R = 1024;


      PWM0_0_CMPB_R = 0;
      PWM0_3_CMPB_R = 0;


      PWM0_0_CTL_R = PWM_0_CTL_ENABLE;
      PWM0_3_CTL_R = PWM_0_CTL_ENABLE;

      PWM0_ENABLE_R = PWM_ENABLE_PWM1EN | PWM_ENABLE_PWM7EN;


      //pre_flag = 1;




                                                           // enable LEDs and pushbuttons

}

int getsUart0(USER_DATA *data)
{
    int count = 0;

    char c;


    while (true)
    {
        c = getcUart0();

        if (c == 8 || c == 127 )  // Is the character a backspace
        {
            if (count > 0)
            {
                count --;
            }

        }
        else if (c ==  13) // Is the character a new line
        {
            data-> buffer[count] = 0;
            return 0;

        }
        else if (c >= 32)  // Is the character a valid input
        {
            data ->buffer[count] = c;
            count ++;

            if (count == MAX_CHARS)
            {
                data ->buffer[count] = 0;
                return 0;

            }
        }




    }
}

void parseFields(USER_DATA *data)
{
    uint8_t i = 0;
    uint8_t field_index = 0;
    bool flag = true;



    while(data->buffer[i])
    {
        if(((data->buffer[i] >= 65 && data->buffer[i] <= 90) || (data->buffer[i] >= 97 && data->buffer[i] <= 122)) ) // Upper case A-Z
        {
            if(flag)

               {
                    data->fieldCount++;
                    data->fieldPosition[field_index] = i;
                    data->fieldType[field_index] = 'a';
                    flag = false;
                    field_index++;
               }

        }
        else if((data->buffer[i] >= 48 && data->buffer[i] <= 57))
        {
               if(flag)
               {
                   data->fieldCount++;
                   data->fieldPosition[field_index] = i;
                   data->fieldType[field_index] = 'n';
                   field_index++;
                   flag = false;
               }
        }
        else
        {
          data->buffer[i] = 0;
          flag = true;
        }

        i++;
    }

}

char * getFieldString(USER_DATA * data, uint8_t fieldNumber)
{
    if(fieldNumber <= (data->fieldCount))
    {
        return &(data->buffer[data->fieldPosition[fieldNumber]]);
    }
    else
    {
        return NULL;
    }
}

int32_t getFieldInteger(USER_DATA * data , uint8_t fieldNumber)
{
    int i = 0;
    int argNum = 0;
    int32_t Basenum = 0;

    if ( (fieldNumber <= (data->fieldCount)) && ((data->fieldType[fieldNumber]) == 'n'))
    {

        char * number = &(data->buffer[data->fieldPosition[fieldNumber]]);

        while(number[i])
        {
            argNum = number[i] - 48;
           if(i)
           {
               Basenum = Basenum *10;
           }
           Basenum += argNum;
           i++;
        }
        return Basenum;

    }
    else
    {
        return 0;
    }



}

bool isCommand(USER_DATA *data, const char strCommand[], uint8_t minArguments)
{
    uint8_t i = 0;

   char * string  = &(data->buffer[data->fieldPosition[0]]);

    while(string[i] && strCommand[i])
    {
        if(data->buffer[i] != strCommand[i])
        {
            return false;

        }
        i++;
    }

    if(((data->fieldCount)-1 >= minArguments))
    {
        return true;
    }
    else
    {
        return false;
    }
    /*

    if(((data->fieldCount)-1) < minArguments)
    {
       return false;
    }
    else
    {
        return true;
    }

    */
}

void RING_TIMER(void)
{

    if((current_level <lower_level))
    {

        GREEN_LED =1;
        waitMicrosecond(2000000);
        GREEN_LED = 0;

        if(alarm_status ==1)
        {
            int j = 0;
            while(j < 5800)
            {

             SPEAKER = 1;
             waitMicrosecond(185);
             SPEAKER = 0;
             waitMicrosecond(185);
             j++;
            }
        }
        else
        {
            SPEAKER = 0;
        }

    }

}



void TimerInterrupt(void)
{

  INPUT = 1;
  waitMicrosecond(10);
  INPUT = 0;


 // GREEN_LED ^= 1;
  TIMER1_TAV_R = 0;
  WTIMER1_ICR_R |= TIMER_ICR_TATOCINT;
  TIMER1_CTL_R |= TIMER_CTL_TAEN;




  COMP_ACINTEN_R |= COMP_ACINTEN_IN0;
  NVIC_EN0_R |= 1 << (INT_COMP0-16);



}






void CompInterrupt(void)
{

  time = TIMER1_TAV_R;


  TIMER1_CTL_R &= ~TIMER_CTL_TAEN;

  int32_t expected_level = expected_lv;
  /*
   * Calculating the amount of water in mL
   *
   *
   * y = mx+b
   *
   * m = 1.02543
   * b = 2498.27
   *
   * thus, x = (ticks -2498.27)/1.02543
   */


  int32_t water_lv = 0;



  //INPUT = 1;

   /*
    * Calculating the amount of water in mL
    *
    *
    * y = mx+b
    *
    * m = 1.02543
    * b = 2498.27
    *
    * thus, x = (ticks -2498.27)/1.02543
    */

   //int32_t water_lv = 100 * ((int) ((float)((((time-2990)/1.057)/100))));




 if(time >= 2311 && time <= 3134)
 {
     water_lv = 0;
    // snprintf(str, sizeof(str),"Water lv is approx 0 mL (%d:ticks)\n", time);
     //  putsUart0(str);

 }
 else if(time >= 3135 && time <= 3295)
 {
     water_lv = 100;
    // snprintf(str, sizeof(str),"Water lv is approx 100 mL (%d:ticks)\n", time);
      //    putsUart0(str);

 }
 else if (time >= 3296 && time <= 3396)
 {
     water_lv = 200;
    // snprintf(str, sizeof(str),"Water lv is approx 200 mL (%d:ticks)\n", time);
         // putsUart0(str);

 }
 else if(time >= 3397 && time <= 3497)
 {
     water_lv = 300;
    // snprintf(str, sizeof(str),"Water lv is approx 300 mL (%d:ticks)\n", time);
       //   putsUart0(str);

 }
 else if(time >= 3498 && time <= 3591)
 {
     water_lv = 400;
   //  snprintf(str, sizeof(str),"Water lv is approx 400 mL (%d:ticks)\n", time);
      //   putsUart0(str);

 }
 else if(time >= 3592)
 {
     water_lv = 500;
    // snprintf(str, sizeof(str),"Water lv is approx 500 mL (%d:ticks)\n", time);
      //  putsUart0(str);

 }
 else
 {

 }




   //COMP_ACINTEN_R &= ~COMP_ACINTEN_IN0;


  if(fill_mode_t == 1 && water_lv < expected_level )
  {

      uint32_t short_t = 2;
      TIMER3_CTL_R &= ~TIMER_CTL_TAEN;
      TIMER3_CFG_R = TIMER_CFG_32_BIT_TIMER;// turn-off timer before reconfiguring           // configure as 32-bit timer (A+B)
      TIMER3_TAMR_R |= TIMER_TAMR_TAMR_1_SHOT|TIMER_TAMR_TACDIR;
      TIMER3_TAILR_R = 40000000*short_t;
      TIMER3_CTL_R |= TIMER_CTL_TAEN; //Turn on interrupts
      TIMER3_IMR_R = TIMER_IMR_TATOIM;  // Turn on timer
      NVIC_EN1_R = 1 << (INT_TIMER3A-16-32);

      PWM0_3_CMPB_R = 1023;



  }

  current_level = water_lv;
  previous_tick = current_tick;
  current_tick = time;


  COMP_ACMIS_R = COMP_ACMIS_IN0;




}

void TIMER3_ALARM(void)
{
    TIMER3_ICR_R |= TIMER_ICR_TATOCINT;

    PWM0_3_CMPB_R = 0;

    TIMER3_CTL_R &= ~TIMER_CTL_TAEN;
    RING_TIMER();


}



void RTC_Interrupt(void)
{
float ratio = 0;
uint32_t time =0;
ratio =readEeprom((16*event_alarm)+2);
time = readEeprom((16*event_alarm)+1);
ratio = (uint32_t)((ratio/100)*1023);

TIMER2_CTL_R &= ~TIMER_CTL_TAEN;
TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;// turn-off timer before reconfiguring           // configure as 32-bit timer (A+B)
TIMER2_TAMR_R |= TIMER_TAMR_TAMR_1_SHOT|TIMER_TAMR_TACDIR;
TIMER2_TAILR_R = 40000000*time;
TIMER2_CTL_R |= TIMER_CTL_TAEN; //Turn on interrupts
TIMER2_IMR_R = TIMER_IMR_TATOIM;  // Turn on timer
NVIC_EN0_R = 1 << (INT_TIMER2A-16);






    PWM0_0_CMPB_R = ratio;
   // PWM0_3_CMPB_R = ratio;



  //HIB_RTCM0_R += 50;
 // HIB_RTCLD_R = 0;

  while((~HIB_CTL_R & HIB_CTL_WRC) );
  HIB_IC_R |= HIB_RIS_RTCALT0;

}

void SetAlarm()
{
    char line[50];
    uint32_t current_time =0;
    uint32_t saved_time = 0;
    uint32_t alarm_num = 0;
    bool T_alarm = false;
    int smallest_index = 86400;
    int found_index = 86400;
    int found_p =0;
    int small_p =0;
    uint32_t alarm = 0;

    while((~HIB_CTL_R & HIB_CTL_WRC) );
    current_time = HIB_RTCC_R;

    saved_time = current_time;

    current_time = (current_time % 86400);

    int i =0;
    for(i = 0; i<10;i++)
    {
      alarm_num = readEeprom((16*i)+3);

      if(alarm_num != 0xFFFFFFFF)
      {
        if(alarm_num < current_time)
        {
          if (alarm_num < smallest_index)
          {
              smallest_index = alarm_num;
              small_p = i;
          }
        }

        if((alarm_num > current_time) && (alarm_num < 86400))
        {
            if(alarm_num < found_index)
            {
                found_index = alarm_num;
                found_p = i;
                T_alarm = true;
            }
        }

      }
    }

    if(T_alarm)
    {

        snprintf(line, sizeof(line),"\n Next alarm is block %d \n", found_p);
        putsUart0(line);
        event_alarm = found_p;
        alarm = saved_time - (saved_time%86400) + found_index;
        while((~HIB_CTL_R & HIB_CTL_WRC) );
        HIB_RTCM0_R = alarm;

    }
    else if((found_p ==0) && (small_p >= 0))
    {
        snprintf(line, sizeof(line),"\n Tomorrow's alarm is block %d \n", small_p);
        putsUart0(line);
        event_alarm = small_p;
        alarm = saved_time + (86400 - saved_time%86400) + smallest_index;
        while((~HIB_CTL_R & HIB_CTL_WRC) );
        HIB_RTCM0_R = alarm;

    }
    else
    {
        snprintf(line, sizeof(line),"\n No alarms scheduled \n");
        putsUart0(line);
    }







}

void Time2_interrupt(void)
{

    TIMER2_ICR_R |= TIMER_ICR_TATOCINT;

    PWM0_0_CMPB_R = 0;
    PWM0_3_CMPB_R = 0;


    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;
    SetAlarm();


}
void MOTION_FUNC(void)
{
    uint32_t period = 2;

    TIMER4_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER4_CFG_R = TIMER_CFG_32_BIT_TIMER;// turn-off timer before reconfiguring           // configure as 32-bit timer (A+B)
    TIMER4_TAMR_R |= TIMER_TAMR_TAMR_1_SHOT|TIMER_TAMR_TACDIR;
    TIMER4_TAILR_R = 40000000*period;
    TIMER4_CTL_R |= TIMER_CTL_TAEN; //Turn on interrupts
    TIMER4_IMR_R = TIMER_IMR_TATOIM;  // Turn on timer
    NVIC_EN2_R = 1 << (INT_TIMER4A-16-64);





}

void TIMER4_INTER(void)
{
    if(fill_mode_t != 1)
    {
      if(SENSOR == 1)
      {

          //putsUart0("Sensor is on\n");
        PWM0_3_CMPB_R = 1023;
        waitMicrosecond(500000);
        PWM0_3_CMPB_R = 0;

      }

    }
       TIMER4_ICR_R |= TIMER_ICR_TATOCINT;


        TIMER4_CTL_R &= ~TIMER_CTL_TAEN;

        if(fill_mode_t != 1)
        {
            MOTION_FUNC();
        }


}




//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void){
    // Initialize hardware
    initHw();

    SetAlarm();



    USER_DATA data;



   while(true)
   {
       bool valid = false;
       data.fieldCount = 0;
       // Get the string from the user
       getsUart0(&data);
       //Echo back to the user of the TTY
       #ifdef DEBUG
       putsUart0(data.buffer);
       putcUart0('\n');
       #endif
       // Parse fields
       parseFields(&data);
       // Echo back to the parsed field data (type and fields)
        #ifdef DEBUG
       uint8_t i;

       for(i = 0; i < data.fieldCount; i++)
       {
           putcUart0(data.fieldType[i]);
           putcUart0('\t');
           putsUart0(&data.buffer[data.fieldPosition[i]]);
           putcUart0('\n');
       }
        #endif
       // Command evaluation
       // set add, data-> add and data are integers
       if(isCommand(&data, "set" ,2))
       {
           int32_t add = getFieldInteger(&data,1);
           int32_t data_5=getFieldInteger(&data,2);
           valid = true;

           // do something with this information
       }

       // alert ON/OFF or alert OFF are the expected commands
       if(isCommand(&data, "alert",1))
       {
           char* mode = getFieldString(&data, 1);

                    //Store mode at block 11, index 1
           if(mode[1] == 'N')  // for auto
            {
               writeEeprom(((16*10)+2), (uint32_t)1); //alert ON
               alarm_status = 1;
               //SPEAKER = 1;


             }
             else
             {
                writeEeprom(((16*10)+2), (uint32_t)0); //alert OFF
                alarm_status = 0;
               // SPEAKER = 0;

             }


           valid = true;

           // process the string with your custom strcmp instruction, then do something
       }
       // Process other commands here
       // Look for error

       if(isCommand(&data, "time", 2))
       {
         // Future code to set the time
           int32_t Hour = getFieldInteger(&data,1);
           int32_t Minutes = getFieldInteger(&data,2);

           int32_t seconds = (Hour *3600) + (Minutes * 60);


           while((~HIB_CTL_R & HIB_CTL_WRC) );
           HIB_RTCLD_R = seconds;



           valid = true;
       }
       else if(isCommand(&data, "time" , 0))
       {
          uint32_t Read_info[4]={};

          char line[100];
          uint32_t time = 0;
          uint32_t hours =0;
          uint32_t minutes = 0;

          while((~HIB_CTL_R & HIB_CTL_WRC) );
          time = HIB_RTCC_R;

          time = (time/60);

          while(time >= 60)
          {
             if(time >= 60)
               {
                 hours++;
               }
             time = time -60;

          }
           minutes = time;


           snprintf(line, sizeof(line),"Time %02d:%02d\n", hours, minutes);
           putsUart0(line);



           int j = 0;
           int i = 0;
           for(j = 0; j<10; j++)
           {
              for(i = 0; i<4;i++)
              {
                  Read_info[i] = readEeprom((16*j)+i);

              }

              if(Read_info[3] !=0xFFFFFFFF )
              {

               uint32_t Secs = Read_info[3];
               hours = (Secs/3600);
               minutes = (Secs%3600)/60;

               snprintf(line, sizeof(line),"\n At Feeding %d, %d: duration, %d: PWM,  %02d:%02d\n", Read_info[0],Read_info[1], Read_info[2],hours,minutes);
               putsUart0(line);
              }
              else
              {
                  snprintf(line, sizeof(line),"\n At Feeding %d, %d: duration, %d: PWM,  %d\n", Read_info[0],Read_info[1], Read_info[2],Read_info[3]);
                  putsUart0(line);

              }


          }


           SetAlarm();

           char * result;
           char * alarm;
           uint32_t water = readEeprom((16*10));
           uint32_t fill_mode = readEeprom((16*10)+1);
           uint32_t  alarm_mode = readEeprom((16*10)+2);

            if(alarm_mode == 1)
             {
                alarm = "ON";
             }
              else
             {
               alarm = "OFF";
             }

           if(fill_mode == 1)
           {
               result = "auto";
           }
           else
           {
               result = "motion";
           }

           snprintf(line, sizeof(line),"\n The water lv is %d ml with fill:%s and alarm:%s", water,result,alarm );
           putsUart0(line);


           fill_mode_t = readEeprom(((16*10)+1));
           expected_lv = readEeprom(((16*10)));

           if(fill_mode_t == 0)
           {
               MOTION_FUNC();
           }


           alarm_status =  readEeprom(((16*10)+2));



           valid = true;

       }
       else if(isCommand(&data, "feed", 5))
       {
           uint32_t blocknum = 0;
           //char sen[50];

           uint32_t Feed_info[4]={};
           uint32_t Hours = 0;
           uint32_t Minutes = 0;
           uint32_t Secs =0;

           blocknum = Feed_info[0] = getFieldInteger(&data, 1); //Feeding event [0,9]
           Feed_info[1] = getFieldInteger(&data,2); //Duration of feeding in seconds
           Feed_info[2] = getFieldInteger(&data,3); //PWM percent rate (TO DO)

           Hours = getFieldInteger(&data,4); // Hour for feeding event
           Minutes = getFieldInteger(&data,5); // Minutes for feeding event

           Secs = (Hours*3600) + (Minutes * 60);
           Feed_info[3] = Secs;
           Hours = 0;
           Minutes = 0;

           int i = 0;
           for (i = 0; i < 4; i++)
           {
               writeEeprom((16*blocknum)+i,Feed_info[i]);
           }


       }
       else if(isCommand(&data,"feed",2))
       {
           uint32_t delete_block = 0;
           uint32_t delete_array[4] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};


           delete_block = getFieldInteger(&data,1);

           int i;

           for(i = 0 ; i<5;i++)
           {
               writeEeprom((16*delete_block) + i, delete_array[i]);
           }


           valid = true;




       }
       else if(isCommand(&data, "VOLUME" , 1))
       {

           uint32_t water = 0;


           water = getFieldInteger(&data,1);

           //Store water at block 11, index 0
           writeEeprom((16*10), water);


           expected_lv = water;
          // expected_lv = readEeprom(((16*10)));






           valid = true;
       }
       else if(isCommand(&data, "fill",1))
       {
          char* mode = getFieldString(&data, 1);

          //Store mode at block 11, index 1
          if(mode[0] == 'a')  // for auto
          {
             writeEeprom(((16*10)+1), (uint32_t)1);
             //fill_mode =1;
             fill_mode_t = readEeprom(((16*10)+1));
             TIMER4_INTER();
          }
          else
          {
             writeEeprom(((16*10)+1), (uint32_t)0); //for motion
             //fill_mode = 0;
             fill_mode_t = readEeprom(((16*10)+1));
             waitMicrosecond(500);
             TIMER3_ALARM();
             MOTION_FUNC();



          }


          //fill_mode_t = readEeprom(((16*10)+1));






          valid = true;
       }
       else if(isCommand(&data, "water",0))
       {
           char sentence[100]={};

           snprintf(sentence, sizeof(sentence),"\n Water LV: %d\n", current_level );
           putsUart0(sentence);

           valid = true;

       }
       else if(isCommand(&data, "minimum", 1))
       {
           uint32_t reading = getFieldInteger(&data,1);
           lower_level = reading;

           char sentence[100]={};

           snprintf(sentence, sizeof(sentence),"\n Lower Lv: %d\n", lower_level );
           putsUart0(sentence);


           valid = true;


       }



       if(!valid)
       {
           putsUart0("Invalid command \n");
       }



   }









}
