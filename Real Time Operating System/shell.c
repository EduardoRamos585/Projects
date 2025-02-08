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
#include "shell.h"
#include "wait.h"

// REQUIRED: Add header files here for your strings functions, ...

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: add processing for the shell commands through the UART here
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

        yield();

    }
}
void hexToNum(uint32_t* num, char * String)
{
    uint8_t i = 0;;
    while(String[i] != NULL)
    {
        char currlett = String[i];
        uint8_t value = 0;

        if((currlett >= '0') && (currlett <= '9'))
        {
            value = currlett - '0';
        }
        else if((currlett >= 'a') && (currlett <= 'f'))
        {
            value = (currlett - 'a') + 10;
        }
        else if((currlett >= 'A') && (currlett <= 'F'))
        {
            value = (currlett - 'A') + 10;
        }
        else
        {
            return;
        }

        *num = (*num << 4) | value;
        i++;
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

        while(number[i])  // For Base 10 Conversion
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
    uint8_t size1 = 0;
    uint8_t size2 = 0;
    char * string  = &(data->buffer[data->fieldPosition[0]]);
    size1 = strLen(string);
    size2 = strLen(strCommand);

    if(size1 != size2)
    {
        return false;
    }

    while(string[i] && strCommand[i])
    {
        if(data->buffer[i] != strCommand[i])
        {
            return false;

        }
        i++;
    }

    if(((data->fieldCount)-1) < minArguments)
    {
       return false;
    }
    else
    {
        return true;
    }
}



void reboot(void)
{
    shellReboot();
}

void ps(void)
{
    shellPs();
}

void ipcs(void)
{
    shellIpcs();
    //putsUart0("\n IPCS called \n");

}

void stringCopy(char * L1,  const char * L2)
{
    uint8_t i = 0;

    while((L1[i] != NULL) || (L2[i] != NULL))
    {
        L1[i] = L2[i];
        i++;
    }

    return;

}

bool stringCompare(char * L1, char* L2)
{
    uint8_t i = 0;
    bool ok = true;;

    while((L1[i] != NULL) || (L2[i] != NULL))
    {
       if(L1[i] != L2[i])
       {
           ok = false;
       }
       i++;
    }
    return ok;

}

void kill(uint32_t pid)
{
    shellKill();

}

void pkill( char name[])
{
   shellPidof();

   uint32_t PID = getR0();

   kill(PID);
}

void pi(bool on)
{
  if(on)
  {
      putsUart0("\n pi on \n");
  }
  else
  {
      putsUart0("\n pi off \n");
  }
}

void preempt(bool on)
{
    shellPreempt();
}

void sched(bool prio_on)
{
    shellSched();
}

void memInfo()
{
    shellMeminfo();
}

void pidof(char name[])
{

    char sent[15];
    shellPidof();
    uint32_t PID = getR0();
    numToString(PID,sent,true,false);
    putsUart0("Pid of Task ");
    putsUart0(name);
    putsUart0(": ");
    putsUart0(sent);
    putsUart0("\n");


}
void shellTask(char name[])
{
    shellTaskRun();
}

void numToString(uint32_t value, char* buffer, bool printHex, bool alreadyHex)
{
    if(printHex && !alreadyHex)
    {
        buffer[0] = '0';
        buffer[1] = 'x';

        int8_t i;

        for(i = 2; i < 10; i++)
        {
            int8_t nibble = value % 16;
            if(nibble < 10)
            {
                buffer[i] = '0' + nibble;
            }
            else
            {
                buffer[i] = 'A' + (nibble - 10);
            }

            value /= 16;

        }

        buffer[10] = '\0';
        uint8_t j;
        uint8_t hex_size = 8;

        for(j = 2; j < 6; j++)
        {
           char temp = buffer[j];
           buffer[j] = buffer[j + hex_size -1];
           buffer[j + hex_size -1] = temp;
           hex_size -= 2;
        }



    }
    else if(!printHex && !alreadyHex)
    {
        if(value == 0)
        {
          buffer[0] = '0';
          buffer[1] = '\0';
          return;
        }

        char string[13];
        uint8_t index = 0;

        while(value > 0)
        {
          string[index++] = (value % 10) + '0';
          value /= 10;
        }

        uint8_t size = index;
        uint8_t i;
        for(i = 0; i < size; i++)
        {
          buffer[i] = string[size - 1 - i];
        }
        buffer[size] = '\0';

    }

    else
    {

        int8_t location = 2;

        buffer[0] = '0';
        buffer[1] = 'x';

        if(value == 0)
        {
            buffer[2] = '0';
            buffer[3] = '\0';
            return;
        }
        else
        {
           while(value > 0)
           {
               uint8_t num = value & 0xF;
               buffer[location++] = (num < 10) ? ('0' + num) : ('A' + (num - 10));
               value >>= 4;
           }

           buffer[location] = '\0';
           uint8_t j;
           uint8_t hex_size = 8;

           for(j = 2; j < 6; j++)
           {
               char temp = buffer[j];
               buffer[j] = buffer[j + hex_size -1];
               buffer[j + hex_size -1] = temp;
               hex_size -= 2;
           }


        }
    }


}

void stringCat(char * sent1, char * sent2, char * result)
{

    uint8_t i = 0;
    while(sent1[i] != NULL)
    {
        result[i] = sent1[i];
        i++;
    }
    uint8_t j = i;
    i = 0;
    while(sent2[i] != NULL)
    {
        result[j] = sent2[i];
        i++;
        j++;
    }
    result[j+1] = '\0';
}

uint8_t strLen(const char * sent)
{
    uint8_t i = 0;
    while(sent[i]!= NULL)
    {
        i++;
    }
    return i;
}


void shell(void)
{
    USER_DATA data;
    bool once = false;
    uint8_t i;
    uint8_t count = 0;

    char Init[] = {"ssh jlosh@192.168.1.24\n"};
    char con[] = {"............\n"};
    char ret[] = {"Retrieving /home/jlosh/RTOS/Fall2023/Solutions\n"};
    char det[] = {"Download complete...Exiting Session\n"};

    while(!once)
    {
        if(count == 0)
        {
            i = 0;
            while(Init[i] != NULL)
            {
                putcUart0(Init[i]);
                sleep(100);

                i++;
            }
        }
        else if(count == 1)
        {
            i = 0;
            while(con[i] != NULL)
            {
                putcUart0(con[i]);
                sleep(100);

                i++;
            }

        }
        else if(count == 2)
        {
            i = 0;
            while(ret[i] != NULL)
            {
                putcUart0(ret[i]);
                sleep(100);

                i++;
            }
        }
        else if(count == 3)
        {
            i = 0;
            while(det[i] != NULL)
            {
                putcUart0(det[i]);
                sleep(100);

                i++;
            }
            once = true;
        }
        count ++;
    }
    putsUart0("Jlosh:~$ ");

        while(true)
        {
          data.fieldCount = 0;

          if(kbhitUart0())
          {
                getsUart0(&data);
                parseFields(&data);


                if(isCommand(&data,"reboot",0))
                {
                   reboot();
                }
                else if(isCommand(&data,"ps",0))
                {
                    ps();
                }
                else if(isCommand(&data,"ipcs",0))
                {
                    ipcs();
                }
                else if(isCommand(&data,"meminfo",0))
                {
                    memInfo();
                }
                else if(isCommand(&data, "kill",1))
                {
                   char * NUM = NULL;
                   uint32_t pid = 0;

                   NUM = getFieldString(&data,1);
                   hexToNum(&pid, NUM);

                   kill(pid);
                   putsUart0("Task :");
                   putsUart0(NUM);
                   putsUart0(" killed\n");

                }
                else if(isCommand(&data,"pkill",1))
                {
                    char name[15];
                    char * index = NULL;
                    uint8_t i = 0;
                    index = getFieldString(&data,1);
                    while((index[i] != 0) && (i < 15))
                    {
                        name[i] = index[i];
                        i++;
                    }
                    name[i] = '\0';
                    pkill(name);
                    putsUart0("Task ");
                    putsUart0(name);
                    putsUart0(" killed\n");

                }
                else if(isCommand(&data,"pidof",1))
                {
                   char name[15];
                   char * index = NULL;
                   uint8_t i = 0;
                   index = getFieldString(&data,1);
                   while((index[i] != 0) && (i < 15))
                  {
                     name[i] = index[i];
                     i++;
                  }
                   name[i] = '\0';
                   pidof(name);
                }
                else if(isCommand(&data,"pi",1))
                {
                    char* mode = NULL;
                    bool pih = false;
                    mode = getFieldString(&data,1);

                    if(mode[1] == 'N')
                    {
                        pih = true;
                    }
                    pi(pih);
                }
                else if(isCommand(&data,"preempt",1))
                {
                   char* point = NULL;
                   bool prmpt = false;
                   point = getFieldString(&data,1);
                   if(point[1] == 'N')
                   {
                       prmpt = true;
                   }
                   preempt(prmpt);
                }
                else if(isCommand(&data,"sched",1))
                {
                  char * schedP = NULL;
                  bool prio = false;
                  schedP = getFieldString(&data,1);
                  if(schedP[0] == 'P')
                  {
                    prio = true;
                  }
                  sched(prio);
                }
                else
                {

                   char * Word = getFieldString(&data,0);
                   char string[15];
                   uint8_t i = 0;
                   while((Word[i] != 0) && (i < 15))
                   {
                       string[i] = Word[i];
                       i++;
                   }
                   string[i] = '\0';
                   shellTask(string);



                }

                putsUart0("\nJlosh:~$ ");

          }


          yield();
      }
}
