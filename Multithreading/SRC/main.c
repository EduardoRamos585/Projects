// MIT License
// 
// Copyright (c) 2023 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <pthread.h>
#include <stdint.h>
#include "utility.h"
#include "star.h"
#include "float.h"
#include <unistd.h>
#include <sys/time.h>

#define NUM_STARS 30000 
#define MAX_LINE 1024
#define DELIMITER " \t\n"

struct Star star_array[NUM_STARS];
uint8_t(*distance_calculated)[NUM_STARS];     // In order to make our program work, we need to set some global variables 
                                              // in so, we can share these variables across 
double min = FLT_MAX;                         // the calculateangulardistance function
double max = FLT_MIN;
pthread_mutex_t mutex;




double mean = 0;

uint64_t count = 0;




int NUMOFWORK = 0;



void showHelp()
{
    printf("Use: findAngular [options]\n");
    printf("Where options are:\n");
    printf("-t          Number of threads to use\n");         // When running the program, the user can use the parameter command
    printf("-h          Show this help\n");                   // -help to activate this function containing instructions.
}

// 
// Embarassingly inefficient, intentionally bad method
// to calculate all entries one another to determine the
// average angular separation between any two stars 
void* determineAverageAngularDistance(void* arg)
{

    uint32_t i, j;

    int k = (int)arg;

    int START = (NUMOFWORK * ((k + 1) - 1));              // Before starting the function, we need to calculate the start and end     
    int END = (NUMOFWORK * (k + 1));                      // points for each thread, we do that by using this formula which takes the 
                                                          // variable numwork and mutiplies it with it's respective k value




    for (i = START; i < END; i++)
    {
        for (j = 0; j < NUM_STARS; j++)
        {

            if (i != j && distance_calculated[i][j] == 0)
            {



                double distance = calculateAngularDistance(star_array[i].RightAscension, star_array[i].Declination,
                                                        star_array[j].RightAscension, star_array[j].Declination);



                distance_calculated[i][j] = 1;
                distance_calculated[j][i] = 1;


                pthread_mutex_lock(&mutex);         // When running the loop, we are using multiple threads to reduce the workload 
                count++;                            // thus, we guard this are where global variables are acessed. In so, we avoid
                                                    // any race conditions.

                if (min > distance)
                {

                    min = distance;


                }

                if (max < distance)
                {

                    max = distance;


                }

                mean = mean + (distance - mean) / count;
                pthread_mutex_unlock(&mutex);



            }




        }





    }








}


int main(int argc, char* argv[] )
{


int NUMTHREAD = 0;
struct timeval begin;  //program variables and variables
struct timeval end;   // for get time of day function.






if(argv[1] == NULL)
{
  NUMTHREAD = 1;
}
else if((strcmp(argv[1], "-help") == 0) && (argv[2] == NULL)) // If the command ./findAngular is written
{                                                             // or the help command is used, thread = 1
  NUMTHREAD = 1;                                      
}


for (int x = 1; x < argc; x++)
{
    if (strcmp(argv[x], "-t") == 0)
    {
        NUMTHREAD = atoi(argv[x + 1]);        //  We read the parameter from -t or for -h to 
                                              //display the correct functions
                                              // then we read the number of threads to be used or assume one thread if none appear
    }


}






FILE* fp;
uint32_t star_count = 0;

pthread_mutex_init(&mutex, NULL);

uint32_t n;







pthread_t threads[NUMTHREAD];
NUMOFWORK = (NUM_STARS / NUMTHREAD);

distance_calculated = malloc(sizeof(uint8_t[NUM_STARS][NUM_STARS]));  // Before reading data, we allocate memeory for the data
                                                                      // as well as setting up the thread array and load of work
if (distance_calculated == NULL)
{
    uint64_t num_stars = NUM_STARS;
    uint64_t size = num_stars * num_stars * sizeof(uint8_t);
    printf("Could not allocate %ld bytes\n", size);
    exit(EXIT_FAILURE);
}

int i, j;
// default every thing to 0 so we calculated the distance.
// This is really inefficient and should be replace by a memset
for (i = 0; i < NUM_STARS; i++)
{
    for (j = 0; j < NUM_STARS; j++)
    {
        distance_calculated[i][j] = 0;
    }
}

for (n = 1; n < argc; n++)
{
    if (strcmp(argv[n], "-help") == 0)
    {
        showHelp();
        exit(0);
    }
}

fp = fopen("data/tycho-trimmed.csv", "r");

if (fp == NULL)
{
    printf("ERROR: Unable to open the file data/tycho-trimmed.csv\n");
    exit(1);
}

char line[MAX_LINE];
while (fgets(line, 1024, fp))     // We read the data 
{                              // from the tycho file then we store the data into the struct for processing 
    uint32_t column = 0;

    char* tok;
    for (tok = strtok(line, " ");
            tok && *tok;
            tok = strtok(NULL, " "))
    {
        switch (column)
        {
            case 0:
                star_array[star_count].ID = atoi(tok);
                break;

            case 1:
                star_array[star_count].RightAscension = atof(tok);
                break;

            case 2:
                star_array[star_count].Declination = atof(tok);
                break;

            default:
                printf("ERROR: line %d had more than 3 columns\n", star_count);
                exit(1);
                break;
        }
        column++;
    }
    star_count++;
}
printf("%d records read\n", star_count);

// Find the average angular distance in the most inefficient way possible



gettimeofday(&begin, NULL);

for (i = 0; i < NUMTHREAD; i++)
{
    pthread_create(&threads[i], NULL, determineAverageAngularDistance, (void*)i);

}                                                         // Once we have read the data needed, and stored in the global struct
                                                          // we create the threads that will run the angulardistancefunction

for (int i = 0; i < NUMTHREAD; i++)
{
    pthread_join(threads[i], NULL);                 // Once we have collected all the threads, the main program can continue it's
}                                                   // execution. This way, we won't lose any precious data from early terminated 
                                                    // threads.
gettimeofday(&end, NULL);
pthread_mutex_destroy(&mutex);



printf("Average distance found is %lf\n", mean);
printf("Minimum distance found is %lf\n", min);     // Once we have completed the mlti-threading section, we stop the timer that
printf("Maximum distance found is %lf\n", max);     // started before creating the threads to measure the time it took to execute





int time_duration = ((end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec));

printf("Duration: %d microseconds\n", time_duration);



return 0;                                                  // Once we have completed all our calculations, we display the data
                                                            // and exit the program 
}