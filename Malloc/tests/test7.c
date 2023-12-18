#include <stdio.h>
#include <stdlib.h>


int main()
{
    int * pointer = NULL;

    pointer = malloc(15 * sizeof(int));

    for(int i =0;i<15;i++)
    {
        *(pointer + i)= i;

        printf("The list of number is %d\n",*(pointer+i));

        
    }

    free(pointer);
}