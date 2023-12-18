#include <stdio.h>
#include <stdlib.h>

#define A 1024

int main()
{

    char *pointer1 = NULL;

    pointer1 = malloc(3*A);


    char* pointer2 = NULL;

    pointer2 = malloc(4*A);

    free(pointer1);

    pointer1 = malloc(4*A);

   

    free(pointer2);
    return 0;
}