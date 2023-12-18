#include <stdlib.h>
#include <stdio.h>


int main()
{
    char* list[100];


    for(int i = 0; i<100; i++)
    {
        list[i] = malloc(10);
        *list[i] = (char)i;
    }

    int result = 0;

    for(int i = 0;i<100;i++)
    {
        result = (int)*list[i] + result;
        free(list[i]);
    }

    printf("The result of the calulation is %d\n",result);
    return 0;
}