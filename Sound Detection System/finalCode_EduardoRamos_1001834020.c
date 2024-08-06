//Eduardo Ramos Lab 4
//  February 27, 2024


// Hibernation Module

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "clock.h"
#include "wait.h"
#include "uart0.h"
//PE2
//PE1
//PD3
//PD2
#define C0_MINUS PORTC,7
#define C0_PLUS PORTC,6
#define T_SPEAKER PORTE,2 // PE2 TALL SPEAKER
#define S_L_SPEAKER PORTE,1 // PE1 LEFT SPEAKER
#define S_BR_SPEAKER PORTE,3 // PD3 SHOR BOTTOM RIGHT SPEAKER
#define S_TR_SPEAKER PORTD,2 // PD2 SHORT TOP RIGHT SPEAKER
#define FIFO_FULL 0x1000
#define VREF_VALUE 0x2
#define SOURCE_INPUT 0x2000000
#define XFERMODE_MASK 0x1

//LEDS
#define RED_LED PORTF,1
#define GREEN_LED PORTF,3
#define BLUE_LED PORTF,2

#define PRIMARY_OFFSET 0x190
#define SRC_TO_DEP_OFFSET 0x4
#define SRC_T_CTRWD_OFFSET 0x8

#define ALTERNATE_OFFSET 0x390


#pragma DATA_ALIGN(arr, 1024)
uint8_t arr[1024];

int16_t BufferOne[128];

int16_t BufferTwo[128];

int16_t W_Buffer[64]; // FIRST SPEAKER (TALL SPEAKER) PE2
int16_t X_Buffer[64]; // SECOND SPEAKER (SHORT LEFT SPEAKER) PE1
int16_t Y_Buffer[64]; // THIRD SPEAKER (SHORT BOTTOM RIGHT SPEAKER) PD3
int16_t Z_Buffer[64]; // FOURTH SPEAKER (SHORT TOP RIGHT SPEAKER) PD2


int16_t G_T_xy;
int16_t G_T_yz;
int16_t G_T_xz;

int16_t Holdoff = 100;


int32_t Angle;

// N - L - 1
// L (Window Size: 6)

bool INT_FLAG = false;
bool DATA_AVAILABLE = false;
bool Always_on = false;
int option = 0;
uint8_t intCount = 0;
uint32_t index = 0;

void Cross_Correlate();
int16_t Calculate_Correlation(int16_t X[], int16_t Y[], int16_t * Index);
int16_t find_MAX(int16_t X, int16_t Y, int16_t Z);



volatile uint32_t* G_DMA_PRIMSRCEP;
volatile uint32_t* G_DMA_PRIMDESEP;
volatile uint32_t* G_DMA_PRIMCTLW;
volatile uint32_t* G_DMA_ALTSRCEP;
volatile uint32_t* G_DMA_ALTDESEP;
volatile uint32_t* G_DMA_ALTCTLW;

void InitHw(void)
{
    initSystemClockTo40Mhz();

    SYSCTL_RCGCACMP_R |= SYSCTL_RCGCACMP_R0;
    SYSCTL_RCGCDMA_R |= SYSCTL_RCGCDMA_R0;
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0 | SYSCTL_RCGCADC_R1;
    _delay_cycles(3);


    enablePort(PORTC);
    enablePort(PORTE);
    enablePort(PORTD);
    enablePort(PORTF);


    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(BLUE_LED);
    selectPinPushPullOutput(RED_LED);
    selectPinAnalogInput(T_SPEAKER);
    selectPinAnalogInput(S_L_SPEAKER);
    selectPinAnalogInput(S_BR_SPEAKER);
    selectPinAnalogInput(S_TR_SPEAKER);

    selectPinAnalogInput(C0_MINUS);


    // Configure Sampler 0
    ADC0_ACTSS_R  &= ~ADC_ACTSS_ASEN1;
    ADC0_EMUX_R   |= ADC_EMUX_EM1_ALWAYS;
    ADC0_SSMUX1_R |= (1 << ADC_SSMUX1_MUX0_S); // PE2 is first sample (tall MIC)
    ADC0_SSMUX1_R |= (2 << ADC_SSMUX1_MUX1_S); // PE1 is second sample ( short left MIC)
    ADC0_SSMUX1_R |= (4 << ADC_SSMUX1_MUX2_S); // PD3 is third sample ( short bottom right MIC)
    ADC0_SSMUX1_R |= (5 << ADC_SSMUX1_MUX3_S); // PD2 is fourth sample ( short top right MIC)
    ADC0_SSCTL1_R |= ADC_SSCTL1_END3; // END on fourth sample
    //ADC0_SAC_R |= ADC_SAC_AVG_64X;


    // Configure Sampler 1
    ADC1_ACTSS_R  &= ~ADC_ACTSS_ASEN1;
    ADC1_EMUX_R   |= ADC_EMUX_EM1_ALWAYS;
    ADC1_SSMUX1_R |= (1 << ADC_SSMUX1_MUX0_S); // PE2 is first sample (tall MIC)
    ADC1_SSMUX1_R |= (2 << ADC_SSMUX1_MUX1_S); // PE1 is second sample ( short left MIC)
    ADC1_SSMUX1_R |= (4 << ADC_SSMUX1_MUX2_S); // PD3 is third sample ( short bottom right MIC)
    ADC1_SSMUX1_R |= (5 << ADC_SSMUX1_MUX3_S); // PD2 is fourth sample ( short top right MIC)
    ADC1_SSCTL1_R |= ADC_SSCTL1_END3 | ADC_SSCTL1_IE3; // END on fourth sample, &  DMA interrupt on fourth sample




    // Set Digital Comparator for four MIC

    ADC0_SSOP1_R |= ADC_SSOP1_S0DCOP; // Sample 0 to Digital Comp operation
    ADC0_SSOP1_R |= ADC_SSOP1_S1DCOP; // Sample 1 to Digital Comp operation
    ADC0_SSOP1_R |= ADC_SSOP1_S2DCOP; // Sample 2 to Digital Comp operation
    ADC0_SSOP1_R |= ADC_SSOP1_S3DCOP; // Sample 3 to Digital Comp operation


    ADC0_SSDC1_R |= (0x0 << 0);  // Sample 0 recieves DCOMP 0
    ADC0_SSDC1_R |= (0x1 << 4); // Sample 1 recives DCOMP 1
    ADC0_SSDC1_R |= (0x2 << 8); // Sample 2 recieves DCOMP 2
    ADC0_SSDC1_R |= (0x3 << 12); // Sample 3 recieves DCOMP 3

    // Configure individual DCOMPx

    // DCOMP 0
    ADC0_DCCTL0_R |= ADC_DCCTL0_CIE;  // Interrupt enable
    ADC0_DCCTL0_R |= ADC_DCCTL0_CIC_HIGH; // High band
    ADC0_DCCTL0_R |= ADC_DCCTL0_CIM_ONCE; // Always send interrupt

    // DCOMP 1
    ADC0_DCCTL1_R |= ADC_DCCTL1_CIE;
    ADC0_DCCTL1_R |= ADC_DCCTL1_CIC_HIGH;
    ADC0_DCCTL1_R |= ADC_DCCTL1_CIM_ONCE;

    // DCOMP 2
   ADC0_DCCTL2_R |= ADC_DCCTL2_CIE;
    ADC0_DCCTL2_R |= ADC_DCCTL2_CIC_HIGH;
    ADC0_DCCTL2_R |= ADC_DCCTL2_CIM_ONCE;

    //DCOMP 3
    ADC0_DCCTL3_R |= ADC_DCCTL3_CIE;
    ADC0_DCCTL3_R |= ADC_DCCTL3_CIC_HIGH;
    ADC0_DCCTL3_R |= ADC_DCCTL3_CIM_ONCE;

    // Enter the comp 0 and comp 1 values for each DCOMPx

    //DCOMP 0
    ADC0_DCCMP0_R |= (0x005 << ADC_DCCMP0_COMP0_S);
    ADC0_DCCMP0_R |= (0x180 << ADC_DCCMP0_COMP1_S);

    //DCOMP 1
    ADC0_DCCMP1_R |= (0x005 << ADC_DCCMP1_COMP0_S);
    ADC0_DCCMP1_R |= (0x180 << ADC_DCCMP1_COMP1_S);

    //DCOMP 2
    ADC0_DCCMP2_R |= (0x005 << ADC_DCCMP2_COMP0_S);
    ADC0_DCCMP2_R |= (0x180 << ADC_DCCMP2_COMP1_S);

     //DCOMP 3
    ADC0_DCCMP3_R |= (0x005 << ADC_DCCMP3_COMP0_S);
    ADC0_DCCMP3_R |= (0x180 << ADC_DCCMP3_COMP1_S);

    ADC0_IM_R |= ADC_IM_DCONSS1;
    NVIC_EN0_R = 1 << (INT_ADC0SS1-16);
    ADC0_DCISC_R |= ADC_DCISC_DCINT1;


    ADC1_IM_R |= ADC_IM_MASK1;
    NVIC_EN1_R = 1 << (INT_ADC1SS1-16-32);
    ADC1_ISC_R |= ADC_IM_MASK1;

    UDMA_CFG_R |= UDMA_CFG_MASTEN;

    UDMA_ENACLR_R |= SOURCE_INPUT;

    UDMA_CTLBASE_R = (uint32_t)&arr[0];

    UDMA_CHMAP3_R |= (0x1 << UDMA_CHMAP3_CH25SEL_S); // Select ADC1 SS1 as input for Channel 3
    UDMA_PRIOCLR_R |= SOURCE_INPUT;     // Set bit 25
    UDMA_ALTCLR_R |= SOURCE_INPUT;
    UDMA_USEBURSTCLR_R |= SOURCE_INPUT;
    UDMA_REQMASKCLR_R |= SOURCE_INPUT;
      //UDMA_USEBURSTSET_R |= SOURCE_INPUT;

    volatile uint32_t * DMA_PRIMSRCEP = (((volatile uint32_t *)(UDMA_CTLBASE_R + PRIMARY_OFFSET)));
    volatile uint32_t * DMA_PRIMDESEP =  (((volatile uint32_t *)(UDMA_CTLBASE_R + PRIMARY_OFFSET + SRC_TO_DEP_OFFSET)));
    volatile uint32_t * DMA_PRIMCTLW =  (((volatile uint32_t *)(UDMA_CTLBASE_R + PRIMARY_OFFSET + SRC_T_CTRWD_OFFSET)));
    volatile uint32_t * DMA_ALTSRCEP =  (((volatile uint32_t *)(UDMA_CTLBASE_R + ALTERNATE_OFFSET)));
    volatile uint32_t * DMA_ALTDESEP =  (((volatile uint32_t *)(UDMA_CTLBASE_R + ALTERNATE_OFFSET + SRC_TO_DEP_OFFSET)));
    volatile uint32_t * DMA_ALTCTLW =  (((volatile uint32_t *)(UDMA_CTLBASE_R + ALTERNATE_OFFSET + SRC_T_CTRWD_OFFSET)));


     *DMA_PRIMSRCEP = (uint32_t)&ADC1_SSFIFO1_R;
     *DMA_PRIMDESEP = (uint32_t)(&BufferOne[127]);
     *DMA_ALTSRCEP =  (uint32_t)&ADC1_SSFIFO1_R;
     *DMA_ALTDESEP = (uint32_t)(&BufferTwo[127]);

     *DMA_PRIMCTLW |= UDMA_CHCTL_DSTINC_16 |UDMA_CHCTL_DSTSIZE_16| UDMA_CHCTL_SRCINC_NONE | UDMA_CHCTL_SRCSIZE_16 | UDMA_CHCTL_ARBSIZE_4 | (127 << 4) | UDMA_CHCTL_XFERMODE_PINGPONG;
     *DMA_ALTCTLW |= UDMA_CHCTL_DSTINC_16 |UDMA_CHCTL_DSTSIZE_16| UDMA_CHCTL_SRCINC_NONE | UDMA_CHCTL_SRCSIZE_16 | UDMA_CHCTL_ARBSIZE_4 |(127 << 4) | UDMA_CHCTL_XFERMODE_PINGPONG;


     G_DMA_PRIMSRCEP = DMA_PRIMSRCEP;
     G_DMA_PRIMDESEP = DMA_PRIMDESEP;
     G_DMA_PRIMCTLW = DMA_PRIMCTLW;
     G_DMA_ALTSRCEP = DMA_ALTSRCEP;
     G_DMA_ALTDESEP = DMA_ALTDESEP;
     G_DMA_ALTCTLW = DMA_ALTCTLW;



     ADC0_ACTSS_R  |= ADC_ACTSS_ASEN1;
     ADC1_ACTSS_R  |= ADC_ACTSS_ASEN1;
     UDMA_ENASET_R |= SOURCE_INPUT;

}

void DMA_INT()
{

        if((*G_DMA_PRIMCTLW & 0x7) == UDMA_CHCTL_XFERMODE_STOP )
        {
            *G_DMA_PRIMSRCEP = (uint32_t)&ADC1_SSFIFO1_R;
            *G_DMA_PRIMDESEP = (uint32_t)(&BufferOne[127]);
           * G_DMA_PRIMCTLW |= UDMA_CHCTL_DSTINC_16 |UDMA_CHCTL_DSTSIZE_16| UDMA_CHCTL_SRCINC_NONE | UDMA_CHCTL_SRCSIZE_16 | UDMA_CHCTL_ARBSIZE_4 | (127 << 4) | UDMA_CHCTL_XFERMODE_PINGPONG;
            index = 0;
            option = 0;
        }

        if((*G_DMA_ALTCTLW & 0x7) == UDMA_CHCTL_XFERMODE_STOP )
        {
                *G_DMA_ALTSRCEP = ( uint32_t)&ADC1_SSFIFO1_R;
                *G_DMA_ALTDESEP = (uint32_t)(&BufferTwo[127]);
                *G_DMA_ALTCTLW |= UDMA_CHCTL_DSTINC_16 |UDMA_CHCTL_DSTSIZE_16| UDMA_CHCTL_SRCINC_NONE | UDMA_CHCTL_SRCSIZE_16 | UDMA_CHCTL_ARBSIZE_4 |(127 << 4) | UDMA_CHCTL_XFERMODE_PINGPONG;
                index = 0;
                option = 1;
         }


    UDMA_CHIS_R |= SOURCE_INPUT;
    ADC1_ISC_R |= ADC_IM_MASK1;
    UDMA_ENASET_R |= SOURCE_INPUT;



}

void DCOMP_INT()
{


    if(intCount == 2)
    {
        INT_FLAG = true;
    }

    if(INT_FLAG)
    {
        waitMicrosecond(114);
        UDMA_ENACLR_R |= SOURCE_INPUT;
        INT_FLAG = false;
        DATA_AVAILABLE = true;
        ADC0_IM_R &= ~ADC_IM_DCONSS1;
        ADC1_IM_R &= ~ADC_IM_MASK1;
        NVIC_DIS0_R = 1 << (INT_ADC0SS1 - 16);
        NVIC_DIS1_R = 1 << (INT_ADC1SS1-16-32);


    }


    ADC0_DCISC_R |= ADC_DCISC_DCINT0 |ADC_DCISC_DCINT1 | ADC_DCISC_DCINT2 | ADC_DCISC_DCINT3;
    intCount++;


}

void processInformation()
{
    uint8_t i;
    if(DATA_AVAILABLE)
    {
        uint8_t modulo;
        uint8_t W = 0;
        uint8_t X = 0;
        uint8_t Y = 0;
        uint8_t Z = 0;

            for(i = 0; i < 127; i++)
            {
               modulo = i % 4;

               if(modulo == 0)
               {
                  W_Buffer[W++] = BufferOne[i];
               }
               else if(modulo == 1)
               {
                   X_Buffer[X++] = BufferOne[i];
               }
               else if(modulo == 2)
               {
                   Y_Buffer[Y++] = BufferOne[i];
               }
               else
               {
                   Z_Buffer[Z++] = BufferOne[i];
               }


            }

            for(i = 0; i < 127; i++)
            {
               modulo = i % 4;

               if(modulo == 0)
               {
                  W_Buffer[W++] = BufferTwo[i];
               }
               else if(modulo == 1)
               {
                  X_Buffer[X++] = BufferTwo[i];
               }
               else if(modulo == 2)
               {
                  Y_Buffer[Y++] = BufferTwo[i];
               }
               else
               {
                  Z_Buffer[Z++] = BufferTwo[i];
               }
            }
         DATA_AVAILABLE = false;
         Cross_Correlate();

        }
}

void Cross_Correlate()
{

  char arr[100];
  int16_t xy_value = 0;
  int16_t Index_xy = 0;
  int16_t yz_value = 0;
  int16_t Index_yz = 0;
  int16_t xz_value = 0;
  int16_t Index_xz = 0;

  xy_value = Calculate_Correlation(X_Buffer,Y_Buffer,&Index_xy);
  yz_value = Calculate_Correlation(Y_Buffer,Z_Buffer,&Index_yz);
  xz_value = Calculate_Correlation(X_Buffer,Z_Buffer,&Index_xz);



// Find Angle here

  int16_t T_xy = (int16_t)Index_xy;
  int16_t T_yz = (int16_t)Index_yz;
  int16_t T_xz = (int16_t)Index_xz;

  G_T_xy =  T_xy;
  G_T_yz = T_yz;
  G_T_xz = T_xz;

// Find the highest time



  int16_t Max_V;
  Max_V= find_MAX(T_xy, T_yz, T_xz);

  if(Max_V == T_xy)
  {
     if(T_xz < T_yz)
     {
         Angle = 360 -(T_xz);
         // order : x z y
         setPinValue(GREEN_LED, 1);
         waitMicrosecond(100000);
         setPinValue(GREEN_LED, 0);

     }
     else
     {
         Angle = 120 + (T_yz);
         //order : y z x
         setPinValue(BLUE_LED, 1);
         waitMicrosecond(100000);
         setPinValue(BLUE_LED, 0);

     }
  }
  else if(Max_V == T_yz)
  {
      if(T_xy < T_xz)
      {
          Angle = 120 - (T_xy);
          // order y x z
          setPinValue(RED_LED,1);
          waitMicrosecond(100000);
          setPinValue(RED_LED,0);

      }
      else
      {
          Angle = 240 + (T_xz);
          // order z x y
          setPinValue(BLUE_LED,1);
          waitMicrosecond(100000);
          setPinValue(BLUE_LED,0);


      }
  }
  else
  {
      if(T_xy < T_yz)
      {
          Angle = 0 + (T_xy);
          // order x y z
          setPinValue(GREEN_LED,1);
          waitMicrosecond(100000);
          setPinValue(GREEN_LED,0);

      }
      else
      {
          Angle = 240 - (T_yz);
          // order z y x
          setPinValue(RED_LED,1);
          waitMicrosecond(10000);
          setPinValue(RED_LED,0);

      }
  }



  waitMicrosecond(10000);
  UDMA_ENASET_R |= SOURCE_INPUT;
  ADC0_IM_R |= ADC_IM_DCONSS1;
  ADC1_IM_R |= ADC_IM_MASK1;
  NVIC_EN0_R = 1 << (INT_ADC0SS1 - 16);
  NVIC_EN1_R = 1 << (INT_ADC1SS1-16-32);
  intCount = 0;


}

int16_t find_MAX(int16_t X, int16_t Y, int16_t Z)
{
    int16_t Max;
    Max = X;

    if(Y > Max)
    {
        Max = Y;
    }
    if(Z > Max)
    {
       Max = Z;
    }

    return Max;
}
int16_t Calculate_Correlation(int16_t X[], int16_t Y[], int16_t * Index)
{
    int16_t sum[57] = {0};
    uint8_t i;
    uint8_t j;

    for(i = 0; i < 57 ; i++)
    {
        for(j = 0; j < 6; j++)
        {
            sum[i] += X[j] * Y[i+j];
        }
    }

    int16_t max = sum[0];

    for(i = 0; i < 57; i++)
    {
        if(sum[i] > max)
        {
            max = sum[i];
            (*Index) = (int16_t)i;
        }
    }

    return max;
}
uint8_t asciiToUint8(const char str[])
{
    uint8_t data;
    if (str[0] == '0' && tolower(str[1]) == 'x')
        sscanf(str, "%hhx", &data);
    else
        sscanf(str, "%hhu", &data);
    return data;
}



void aoaDisplay()
{
    Always_on = true;
}

void tdaDisplay()
{
    char arr[100];

    snprintf(arr, sizeof(arr), "T_xy : %d, T_yz : %d T_xz : %d\n", G_T_xy, G_T_yz, G_T_xz);
    putsUart0(arr);

}

void holdoffDisplay()
{
    char arr[100];
    snprintf(arr, sizeof(arr), " Hold off: %d\n",Holdoff);
    putsUart0(arr);
}

#define MAX_CHARS 80
char strInput[MAX_CHARS+1];
char* token;
uint8_t count = 0;

void processShell()
{
    bool end;
    char c;

    if (kbhitUart0())
    {
        c = getcUart0();

        end = (c == 13) || (count == MAX_CHARS);
        if (!end)
        {
            if ((c == 8 || c == 127) && count > 0)
                count--;
            if (c >= ' ' && c < 127)
                strInput[count++] = c;
        }
        else
        {
            strInput[count] = '\0';
            count = 0;
            token = strtok(strInput, " ");
            if (strcmp(token, "tda") == 0)
            {
                 tdaDisplay();
            }
            if (strcmp(token, "aoa") == 0)
            {
                aoaDisplay();
            }
            if (strcmp(token, "Holdoff") == 0)
            {
                token = strtok(NULL, " ");
                if(token == NULL)
                {
                    Holdoff = 0;
                }
                else
                {
                    Holdoff = asciiToUint8(token);
                }
            }

        }
    }
}



int main(void)
{
    InitHw();
    initUart0();
    setUart0BaudRate(115200,40e6);

   // uint8_t i;
    char arr[100];


    while(true){
        processInformation();

        processShell();

       if(Always_on)
       {
          snprintf(arr, sizeof(arr), "Angle: %d",Angle);
         putsUart0(arr);
          putcUart0('\n');

        }

    }
}

